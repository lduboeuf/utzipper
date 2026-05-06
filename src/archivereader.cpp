/*
 * Copyright (C) 2021  Lionel Duboeuf
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * utzip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QDebug>
#include <QStandardPaths>
#include <QUrl>
#include <QDir>
#include <QDirIterator>
#include <algorithm>
#include <archive.h>
#include <archive_entry.h>

#include "archivereader.h"
#include "archiveitem.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  #define SKIP_EMPTY Qt::SkipEmptyParts
#else
  #define SKIP_EMPTY QString::SkipEmptyParts
#endif

namespace {

ArchiveReader::Errors toReaderError(ArchiveOpenError error)
{
    switch (error) {
    case ArchiveOpenError::PasswordRequired:
        return ArchiveReader::ERROR_PASSPHRASE_REQUIRED;
    case ArchiveOpenError::InvalidPassword:
        return ArchiveReader::ERROR_INVALID_PASSPHRASE;
    case ArchiveOpenError::EncryptionUnsupported:
        return ArchiveReader::ERROR_ENCRYPTION_UNSUPPORTED;
    case ArchiveOpenError::ReadError:
        return ArchiveReader::ERROR_READ;
    case ArchiveOpenError::UnsupportedFormat:
        return ArchiveReader::UNSUPPORTED_FILE_FORMAT;
    case ArchiveOpenError::NoError:
    default:
        return ArchiveReader::NO_ERRORS;
    }
}

QString fallbackReaderMessage(ArchiveOpenError error)
{
    switch (error) {
    case ArchiveOpenError::PasswordRequired:
        return QStringLiteral("This ZIP archive requires a passphrase.");
    case ArchiveOpenError::InvalidPassword:
        return QStringLiteral("Incorrect passphrase.");
    case ArchiveOpenError::EncryptionUnsupported:
        return QStringLiteral("This encrypted archive cannot be opened with the current libarchive build.");
    case ArchiveOpenError::UnsupportedFormat:
        return QStringLiteral("Unsupported archive format.");
    case ArchiveOpenError::ReadError:
        return QStringLiteral("Could not read the archive.");
    case ArchiveOpenError::NoError:
    default:
        return QString();
    }
}

}

ArchiveReader::ArchiveReader(QObject *parent) : QAbstractListModel(parent), mHasFiles(false), mRequiresPassphrase(false), mEncrypted(false), mError(NO_ERRORS) {
    connect(this, SIGNAL(archiveChanged()),this, SLOT(extract()));
    connect(this, SIGNAL(rowCountChanged()),this, SLOT(onRowCountChanged()));
}

ArchiveReader::~ArchiveReader()
{
}

QUrl ArchiveReader::archive() const
{
    return mArchive;
}

void ArchiveReader::setArchive(const QUrl &path)
{
    if (mArchive == path) {
        return;
    }
    qDebug() << "new archive:" << path;
    mArchive = path;
    if (!mPassphrase.isEmpty()) {
        mPassphrase.clear();
        Q_EMIT passphraseChanged();
    }
    Q_EMIT archiveChanged();
}

QString ArchiveReader::name() const
{
    return mName;
}

QString ArchiveReader::passphrase() const
{
    return mPassphrase;
}

void ArchiveReader::setPassphrase(const QString &passphrase)
{
    if (mPassphrase == passphrase) {
        return;
    }

    mPassphrase = passphrase;
    Q_EMIT passphraseChanged();
}

bool ArchiveReader::hasFiles() const
{
    return mHasFiles;
}

bool ArchiveReader::requiresPassphrase() const
{
    return mRequiresPassphrase;
}

bool ArchiveReader::encrypted() const
{
    return mEncrypted;
}

QString ArchiveReader::currentDir() const
{
    return mCurrentDir;
}

void ArchiveReader::setCurrentDir(const QString &currentDir)
{
//    if (mCurrentDir == currentDir) {
//        return;
//    }
    qDebug() << "currentDir:" << currentDir;
    mCurrentDir = currentDir;
    Q_EMIT currentDirChanged();

    beginResetModel();
    mCurrentArchiveItems.clear();
    mCurrentArchiveItems = mArchiveItems.value(mCurrentDir);
    endResetModel();

    Q_EMIT rowCountChanged();
}

ArchiveReader::Errors ArchiveReader::error() const
{
    return mError;
}

QString ArchiveReader::errorMessage() const
{
    return mErrorMessage;
}

void ArchiveReader::clear()
{
    mArchive = "";
    mName = "";
    if (!mPassphrase.isEmpty()) {
        mPassphrase.clear();
        Q_EMIT passphraseChanged();
    }
    setError(Errors::NO_ERRORS);
    setErrorMessage(QString());
    setRequiresPassphrase(false);
    setEncrypted(false);
    beginResetModel();
    mCurrentArchiveItems.clear();
    mArchiveItems.clear();
    endResetModel();

    Q_EMIT rowCountChanged();
}

bool ArchiveReader::hasData() const
{
    return mArchiveItems.count() > 0;
}

void ArchiveReader::retry()
{
    extract();
}

void ArchiveReader::cleanDirectory(const QString &path)
{

    QDir dir(path);
    dir.setFilter( QDir::NoDotAndDotDot | QDir::Files | QDir::Hidden );
    foreach( QString dirItem, dir.entryList() )
        dir.remove( dirItem );

    dir.setFilter( QDir::NoDotAndDotDot | QDir::Dirs );
    foreach( QString dirItem, dir.entryList() )
    {
        QDir subDir( dir.absoluteFilePath( dirItem ) );
        subDir.removeRecursively();
    }
}

void ArchiveReader::extract()
{

    setError(Errors::NO_ERRORS);
    setErrorMessage(QString());
    setRequiresPassphrase(false);
    setEncrypted(false);

    QFileInfo info(mArchive.toLocalFile());
    if (!info.isReadable()) {
        qWarning() << "ArchiveReader: Cannot read " << mArchive.toLocalFile();
        setError(Errors::ERROR_READ);
        setErrorMessage(QStringLiteral("Cannot read the selected archive."));
        return;
    }

    mName = info.fileName();
    Q_EMIT nameChanged();

    beginResetModel();
    mCurrentArchiveItems.clear();
    mArchiveItems.clear();
    endResetModel();

    ArchiveReadHandle handle;
    QString errorMessage;
    ArchiveOpenError openError = ArchiveOpenError::NoError;
    if (!openArchiveForReading(mArchive.toLocalFile(), handle, mPassphrase, &openError, &errorMessage)) {
        qWarning() << "Cannot open archive" << mArchive.toLocalFile() << errorMessage;
        setEncrypted(openError == ArchiveOpenError::PasswordRequired
                     || openError == ArchiveOpenError::InvalidPassword
                     || openError == ArchiveOpenError::EncryptionUnsupported);
        setRequiresPassphrase(openError == ArchiveOpenError::PasswordRequired || openError == ArchiveOpenError::InvalidPassword);
        setError(toReaderError(openError));
        setErrorMessage(errorMessage.isEmpty() ? fallbackReaderMessage(openError) : errorMessage);
        return;
    }

    setEncrypted(handle.encrypted);

    struct archive_entry *entry = nullptr;
    ArchiveOpenError readError = ArchiveOpenError::NoError;
    QString readErrorMessage;
    bool success = true;
    while (true) {
        const int headerResult = archive_read_next_header(handle.reader, &entry);
        if (headerResult == ARCHIVE_EOF) {
            break;
        }
        if (headerResult != ARCHIVE_OK) {
            readErrorMessage = archiveErrorString(handle.reader);
            readError = archiveReadError(handle.reader, mPassphrase, readErrorMessage);
            success = false;
            break;
        }

        if (archive_entry_is_encrypted(entry) == 1) {
            setEncrypted(true);
            if (mPassphrase.isEmpty()) {
                readError = ArchiveOpenError::PasswordRequired;
                readErrorMessage = fallbackReaderMessage(readError);
                success = false;
                break;
            }
        }

        const char *entryPath = archive_entry_pathname(entry);
        if (entryPath) {
            const bool isDir = archive_entry_filetype(entry) == AE_IFDIR;
            addArchiveEntry(QString::fromUtf8(entryPath), isDir);
        }

        // For encrypted entries, we must actually read the data (not just skip it)
        // because libarchive only validates the passphrase during real data decryption.
        if (archive_entry_is_encrypted(entry) == 1 && !mPassphrase.isEmpty()) {
            char buf[8192];
            la_ssize_t bytesRead = 0;
            bool readOk = true;
            while ((bytesRead = archive_read_data(handle.reader, buf, sizeof(buf))) > 0) {
                // discard data, we just need to trigger passphrase validation
            }
            if (bytesRead < 0) {
                readErrorMessage = archiveErrorString(handle.reader);
                readError = ArchiveOpenError::InvalidPassword;
                readOk = false;
            }
            if (!readOk) {
                success = false;
                break;
            }
        } else {
            if (archive_read_data_skip(handle.reader) != ARCHIVE_OK) {
                readErrorMessage = archiveErrorString(handle.reader);
                readError = archiveReadError(handle.reader, mPassphrase, readErrorMessage);
                success = false;
                break;
            }
        }
    }

    closeArchiveReader(handle);

    if (!success) {
        beginResetModel();
        mCurrentArchiveItems.clear();
        mArchiveItems.clear();
        endResetModel();
        setRequiresPassphrase(readError == ArchiveOpenError::PasswordRequired || readError == ArchiveOpenError::InvalidPassword);
        setEncrypted(mEncrypted || readError == ArchiveOpenError::PasswordRequired
                     || readError == ArchiveOpenError::InvalidPassword
                     || readError == ArchiveOpenError::EncryptionUnsupported);
        setError(toReaderError(readError));
        setErrorMessage(readErrorMessage.isEmpty() ? fallbackReaderMessage(readError) : readErrorMessage);
        Q_EMIT rowCountChanged();
        return;
    }

    sortArchiveItems();

    Q_EMIT modelChanged();
    setCurrentDir("");
}

void ArchiveReader::onRowCountChanged()
{
    bool containsFile = false;
    foreach(ArchiveItem item, mCurrentArchiveItems)
    {
        if(!item.isDir())
        {
            containsFile = true;
            break;
        }
    }
    if (mHasFiles != containsFile) {
        mHasFiles = containsFile;
        Q_EMIT hasFilesChanged();
    }

}

void ArchiveReader::setError(const ArchiveReader::Errors &error)
{
    mError = error;
    Q_EMIT errorChanged();
}

void ArchiveReader::setErrorMessage(const QString &message)
{
    if (mErrorMessage == message) {
        return;
    }

    mErrorMessage = message;
    Q_EMIT errorMessageChanged();
}

void ArchiveReader::setRequiresPassphrase(bool required)
{
    if (mRequiresPassphrase == required) {
        return;
    }

    mRequiresPassphrase = required;
    Q_EMIT requiresPassphraseChanged();
}

void ArchiveReader::setEncrypted(bool encrypted)
{
    if (mEncrypted == encrypted) {
        return;
    }

    mEncrypted = encrypted;
    Q_EMIT encryptedChanged();
}

void ArchiveReader::addArchiveEntry(const QString &entryPath, bool isDir)
{
    QString normalized = QDir::cleanPath(entryPath);
    if (normalized == ".") {
        return;
    }
    while (normalized.startsWith('/')) {
        normalized.remove(0, 1);
    }
    while (normalized.endsWith('/')) {
        normalized.chop(1);
    }
    if (normalized.isEmpty()) {
        return;
    }

    const QStringList parts = normalized.split('/', SKIP_EMPTY);
    QString parentPath;
    for (int i = 0; i < parts.count(); ++i) {
        const QString &part = parts.at(i);
        const QString currentPath = parentPath.isEmpty() ? part : parentPath + "/" + part;
        const bool partIsDir = (i < parts.count() - 1) || isDir;

        QList<ArchiveItem> &children = mArchiveItems[parentPath];
        bool exists = false;
        const QList<ArchiveItem> existingChildren = children;
        for (const ArchiveItem &item : existingChildren) {
            if (item.name() == part && item.isDir() == partIsDir) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            children << ArchiveItem(part, partIsDir, QUrl::fromLocalFile(currentPath));
        }

        if (partIsDir && !mArchiveItems.contains(currentPath)) {
            mArchiveItems.insert(currentPath, {});
        }
        parentPath = currentPath;
    }
}

void ArchiveReader::sortArchiveItems()
{
    for (auto it = mArchiveItems.begin(); it != mArchiveItems.end(); ++it) {
        auto &items = it.value();
        std::sort(items.begin(), items.end(), [](const ArchiveItem &left, const ArchiveItem &right) {
            if (left.isDir() != right.isDir()) {
                return left.isDir();
            }
            return left.name().compare(right.name(), Qt::CaseInsensitive) < 0;
        });
    }
    if (!mArchiveItems.contains("")) {
        mArchiveItems.insert("", {});
    }
}

QVariantMap ArchiveReader::get(int i) const
{
    QVariantMap archiveItem;
    QHash<int, QByteArray> roles = roleNames();

    QModelIndex modelIndex = index(i, 0);
    if (modelIndex.isValid()) {
        Q_FOREACH(int role, roles.keys()) {
            QString roleName = QString::fromUtf8(roles.value(role));
            archiveItem.insert(roleName, data(modelIndex, role));
        }
    }
    return archiveItem;
}

QHash<int, QByteArray> ArchiveReader::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[IsDirRole] = "isDir";
    roles[FullPathRole] = "fullPath";
    return roles;
}

QVariant ArchiveReader::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= mCurrentArchiveItems.count())
        return QVariant();


    const ArchiveItem &ArchiveItem = mCurrentArchiveItems[index.row()];
    if (role == NameRole)
        return QVariant::fromValue(ArchiveItem.name());
    else if (role == IsDirRole)
        return QVariant::fromValue(ArchiveItem.isDir());
    else if (role == FullPathRole)
        return QVariant::fromValue(ArchiveItem.fullPath());
    else
        return QVariant();
}

int ArchiveReader::rowCount(const QModelIndex & parent) const {
    Q_UNUSED(parent);
    return mCurrentArchiveItems.count();
}

