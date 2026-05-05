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

ArchiveReader::ArchiveReader(QObject *parent) : QAbstractListModel(parent), mError(NO_ERRORS), mHasFiles(false) {
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
    Q_EMIT archiveChanged();
}

QString ArchiveReader::name() const
{
    return mName;
}

bool ArchiveReader::hasFiles() const
{
    return mHasFiles;
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

void ArchiveReader::clear()
{
    mArchive = "";
    mName = "";
    setError(Errors::NO_ERRORS);
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

    QFileInfo info(mArchive.toLocalFile());
    if (!info.isReadable()) {
        qWarning() << "ArchiveReader: Cannot read " << mArchive.toLocalFile();
        setError(Errors::ERROR_READ);
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
    if (!openArchiveForReading(mArchive.toLocalFile(), handle, &errorMessage)) {
        qWarning() << "Cannot open archive" << mArchive.toLocalFile() << errorMessage;
        setError(Errors::UNSUPPORTED_FILE_FORMAT);
        return;
    }

    struct archive_entry *entry = nullptr;
    while (archive_read_next_header(handle.reader, &entry) == ARCHIVE_OK) {
        const char *entryPath = archive_entry_pathname(entry);
        if (entryPath) {
            const bool isDir = archive_entry_filetype(entry) == AE_IFDIR;
            addArchiveEntry(QString::fromUtf8(entryPath), isDir);
        }
        archive_read_data_skip(handle.reader);
    }

    closeArchiveReader(handle);
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

