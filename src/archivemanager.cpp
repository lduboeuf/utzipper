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
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QUrl>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QStringList>
#include <archive.h>
#include <archive_entry.h>

#include "archivemanager.h"
#include "archiveitem.h"

namespace {

QString normalizeArchivePath(const QString &rawPath)
{
    QString normalized = QDir::cleanPath(rawPath);
    if (normalized == ".") {
        return QString();
    }
    while (normalized.startsWith('/')) {
        normalized.remove(0, 1);
    }
    return normalized;
}

bool configureArchiveWriter(struct archive *writer, const QString &suffix)
{
    if (suffix == "zip") {
        return archive_write_set_format_zip(writer) == ARCHIVE_OK;
    }

    if (suffix == "tar") {
        return archive_write_set_format_pax_restricted(writer) == ARCHIVE_OK;
    }

    if (suffix == "tar.gz") {
        return archive_write_add_filter_gzip(writer) == ARCHIVE_OK
            && archive_write_set_format_pax_restricted(writer) == ARCHIVE_OK;
    }

    if (suffix == "tar.bz2") {
        return archive_write_add_filter_bzip2(writer) == ARCHIVE_OK
            && archive_write_set_format_pax_restricted(writer) == ARCHIVE_OK;
    }

    if (suffix == "tar.xz") {
        return archive_write_add_filter_xz(writer) == ARCHIVE_OK
            && archive_write_set_format_pax_restricted(writer) == ARCHIVE_OK;
    }

    if (suffix == "ar") {
        return archive_write_set_format_ar_svr4(writer) == ARCHIVE_OK;
    }

    if (suffix == "7z") {
        return archive_write_set_format_7zip(writer) == ARCHIVE_OK;
    }

    return false;
}

}

ArchiveManager::ArchiveManager(QObject *parent) : QObject(parent), mError(NO_ERRORS) {

    connect(this,SIGNAL(currentDirChanged()),this,SIGNAL(currentNameChanged()));

    // working directory for new archives
    QString output = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/newArchive";
    //QDir::mkpath(output);
    setNewArchiveDir(QUrl::fromLocalFile(output));

    // temp dir
    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    setTempDir(QUrl::fromLocalFile(tmpDir));
}

ArchiveManager::~ArchiveManager()
{
}

QUrl ArchiveManager::currentDir() const
{
    return mCurrentDir;
}

void ArchiveManager::setCurrentDir(const QUrl &currentDir)
{
    if (mCurrentDir == currentDir) {
        return;
    }
    qDebug() << "currentDir:" << currentDir;
    mCurrentDir = currentDir;
    Q_EMIT currentDirChanged();
}

QString ArchiveManager::currentName() const
{
    return mCurrentDir == mNewArchiveDir ? "" : mCurrentDir.fileName();
}

QUrl ArchiveManager::tempDir() const
{
    return mTempDir;
}

void ArchiveManager::setTempDir(const QUrl &path)
{
    if (mTempDir != path) {
        mTempDir = path;
        qDebug() << "tempDir" << path;
        Q_EMIT tempDirChanged();
    }
}

QUrl ArchiveManager::newArchiveDir() const
{
    return mNewArchiveDir;
}

void ArchiveManager::setNewArchiveDir(const QUrl &path)
{
    if (mNewArchiveDir != path) {
        mNewArchiveDir = path;
        mCurrentDir = path;
        qDebug() << "mNewArchiveDir" << path;
        Q_EMIT newArchiveDirChanged();
        Q_EMIT currentDirChanged();
    }
}

ArchiveManager::Errors ArchiveManager::error() const
{
    return mError;
}

void ArchiveManager::setError(const ArchiveManager::Errors &error)
{
    mError = error;
    Q_EMIT errorChanged();
}

void ArchiveManager::clear()
{
    setError(Errors::NO_ERRORS);

    //clean new archive dir
    cleanDirectory(mNewArchiveDir.toLocalFile());

    //clean tmp dir
    cleanDirectory(mTempDir.toLocalFile());

}


QList<QUrl> ArchiveManager::extractFiles(const QUrl &archive, const QList<QUrl> &files)
{
    QList<QUrl> outFiles;
    QFileInfo info(archive.toLocalFile());
    if (!info.isReadable()) {
        setError(Errors::ERROR_READ);
        return outFiles;
    }

    QSet<QString> requestedFiles;
    foreach (const QUrl &path, files) {
        const QString normalizedPath = normalizeArchivePath(path.toLocalFile());
        if (!normalizedPath.isEmpty()) {
            requestedFiles.insert(normalizedPath);
        }
    }

    ArchiveReadHandle handle;
    QString errorMessage;
    if (!openArchiveForReading(archive.toLocalFile(), handle, &errorMessage)) {
        qWarning() << "Cannot open archive" << archive.toLocalFile() << errorMessage;
        setError(Errors::UNSUPPORTED_FILE_FORMAT);
        return outFiles;
    }

    struct archive_entry *entry = nullptr;
    while (archive_read_next_header(handle.reader, &entry) == ARCHIVE_OK) {
        const char *entryRawPath = archive_entry_pathname(entry);
        if (!entryRawPath) {
            archive_read_data_skip(handle.reader);
            continue;
        }
        const QString entryPath = normalizeArchivePath(QString::fromUtf8(entryRawPath));
        if (!requestedFiles.contains(entryPath) || archive_entry_filetype(entry) == AE_IFDIR) {
            archive_read_data_skip(handle.reader);
            continue;
        }

        const QString outPath = mTempDir.toLocalFile() + "/" + entryPath;
        QDir().mkpath(QFileInfo(outPath).absolutePath());
        QFile outputFile(outPath);
        if (!outputFile.open(QIODevice::WriteOnly)) {
            setError(Errors::ERROR_WRITE);
            archive_read_data_skip(handle.reader);
            continue;
        }

        const size_t chunkSize = 8192;
        char buffer[chunkSize];
        la_ssize_t bytesRead = 0;
        while ((bytesRead = archive_read_data(handle.reader, buffer, chunkSize)) > 0) {
            outputFile.write(buffer, bytesRead);
        }
        outputFile.close();
        if (bytesRead < 0) {
            setError(Errors::ERROR_READ);
            continue;
        }

        outFiles << QUrl::fromLocalFile(outPath);
    }

    closeArchiveReader(handle);

    qDebug() << outFiles;

    return outFiles;
}

/**
 * Extract the archive in the path folder
 */
void ArchiveManager::extractTo(const QUrl &archive, const QUrl &path)
{
    QFileInfo info(archive.toLocalFile());
    if (!info.isReadable()) {
        setError(Errors::ERROR_READ);
        return;
    }

    ArchiveReadHandle handle;
    struct archive *writer = archive_write_disk_new();
    QString errorMessage;
    if (!openArchiveForReading(archive.toLocalFile(), handle, &errorMessage)) {
        qWarning() << "Cannot open archive" << archive.toLocalFile() << errorMessage;
        setError(Errors::UNSUPPORTED_FILE_FORMAT);
        archive_write_free(writer);
        return;
    }

    archive_write_disk_set_options(writer, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS);
    archive_write_disk_set_standard_lookup(writer);

    struct archive_entry *entry = nullptr;
    while (archive_read_next_header(handle.reader, &entry) == ARCHIVE_OK) {
        const char *entryRawPath = archive_entry_pathname(entry);
        if (!entryRawPath) {
            archive_read_data_skip(handle.reader);
            continue;
        }
        const QString entryPath = normalizeArchivePath(QString::fromUtf8(entryRawPath));
        if (entryPath.isEmpty()) {
            archive_read_data_skip(handle.reader);
            continue;
        }

        const QString fullPath = path.toLocalFile() + "/" + entryPath;
        archive_entry_set_pathname(entry, fullPath.toUtf8().constData());

        int result = archive_write_header(writer, entry);
        if (result != ARCHIVE_OK) {
            qWarning() << "Cannot extract entry" << entryPath << archive_error_string(writer);
            archive_read_data_skip(handle.reader);
            continue;
        }

        const size_t chunkSize = 8192;
        char buffer[chunkSize];
        la_ssize_t bytesRead = 0;
        while ((bytesRead = archive_read_data(handle.reader, buffer, chunkSize)) > 0) {
            if (archive_write_data(writer, buffer, bytesRead) < 0) {
                setError(Errors::ERROR_WRITE);
                break;
            }
        }
        archive_write_finish_entry(writer);
    }

    archive_write_close(writer);
    archive_write_free(writer);
    closeArchiveReader(handle);

    setCurrentDir(mNewArchiveDir);
}

bool ArchiveManager::isArchiveFile(const QUrl &path)
{
    if (!path.isValid()) {
        qWarning() << "invalid url:" << path;
        return false;
    }

    return isSupportedArchiveFile(path.toLocalFile());
}

bool ArchiveManager::removeFile(const QUrl &file)
{

    QFile fileToRemove(file.toLocalFile());
    return fileToRemove.remove();
}

bool ArchiveManager::appendFolder(const QString &name, const QUrl &dir)
{

        //const QString key = parentFolder.isEmpty() ? name : parentFolder + "/" + name;
        QString out = dir.path().append("/").append(name);
        qDebug() << "new folder:" << out;
        return QDir().mkdir(out);
}

bool ArchiveManager::removeFolder(const QUrl &folder)
{
    QDir f(folder.toLocalFile());
    return f.removeRecursively();
}

QUrl ArchiveManager::save(const QString &archiveName, const QString &suffix)
{
    QString output = mTempDir.path().append("/").append(archiveName).append(".").append(suffix);
    qDebug() << "save to:" << output;
    struct archive *writer = archive_write_new();
    const bool isArFormat = (suffix == "ar");
    QSet<QString> usedArEntryNames;

    if (!configureArchiveWriter(writer, suffix)) {
        qWarning() << "ERROR. COMPRESSED FILE TYPE UNKOWN " << output;
        setError(Errors::UNSUPPORTED_FILE_FORMAT);
        archive_write_free(writer);
        return QUrl("");
    }

    if (archive_write_open_filename(writer, output.toUtf8().constData()) != ARCHIVE_OK) {
        setError(Errors::ERROR_WRITE);
        qWarning() << "could not open archive for writing" << archive_error_string(writer);
        archive_write_free(writer);
        return QUrl("");
    }

    QDirIterator it(mNewArchiveDir.toLocalFile(), QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString absolutePath = it.next();
        const QFileInfo info(absolutePath);
        const QString relativePath = QDir(mNewArchiveDir.toLocalFile()).relativeFilePath(absolutePath);

        if (isArFormat && info.isDir()) {
            // AR stores file members only; directory entries make the export fail.
            continue;
        }

        if (isArFormat && relativePath.contains('/')) {
            qWarning() << "AR export does not support nested paths:" << relativePath;
            setError(Errors::UNSUPPORTED_FILE_FORMAT);
            archive_write_close(writer);
            archive_write_free(writer);
            return QUrl("");
        }

        if (isArFormat && usedArEntryNames.contains(relativePath)) {
            qWarning() << "AR export duplicate member name:" << relativePath;
            setError(Errors::ERROR_WRITE);
            archive_write_close(writer);
            archive_write_free(writer);
            return QUrl("");
        }

        struct archive_entry *entry = archive_entry_new();
        archive_entry_set_pathname(entry, relativePath.toUtf8().constData());

        if (info.isDir()) {
            archive_entry_set_filetype(entry, AE_IFDIR);
            archive_entry_set_perm(entry, 0755);
            archive_entry_set_size(entry, 0);
            if (archive_write_header(writer, entry) != ARCHIVE_OK) {
                qWarning() << "Cannot write archive header for" << relativePath << archive_error_string(writer);
                archive_entry_free(entry);
                setError(Errors::ERROR_WRITE);
                archive_write_close(writer);
                archive_write_free(writer);
                return QUrl("");
            }
            archive_entry_free(entry);
            continue;
        }

        if (isArFormat) {
            usedArEntryNames.insert(relativePath);
        }

        QFile inputFile(absolutePath);
        if (!inputFile.open(QIODevice::ReadOnly)) {
            archive_entry_free(entry);
            setError(Errors::ERROR_READ);
            archive_write_close(writer);
            archive_write_free(writer);
            return QUrl("");
        }

        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_entry_set_size(entry, info.size());

        if (archive_write_header(writer, entry) != ARCHIVE_OK) {
            qWarning() << "Cannot write archive header for" << relativePath << archive_error_string(writer);
            inputFile.close();
            archive_entry_free(entry);
            setError(Errors::ERROR_WRITE);
            archive_write_close(writer);
            archive_write_free(writer);
            return QUrl("");
        }

        while (!inputFile.atEnd()) {
            const QByteArray data = inputFile.read(8192);
            if (archive_write_data(writer, data.constData(), static_cast<size_t>(data.size())) < 0) {
                inputFile.close();
                archive_entry_free(entry);
                setError(Errors::ERROR_WRITE);
                archive_write_close(writer);
                archive_write_free(writer);
                return QUrl("");
            }
        }
        inputFile.close();
        archive_entry_free(entry);
    }

    archive_write_close(writer);
    archive_write_free(writer);
    qDebug() << "archive copied to:" << output;

    return QUrl::fromLocalFile(output);
}

bool ArchiveManager::isWriteFormatSupported(const QString &suffix) const
{
    static const QStringList supportedFormats = {
        "zip",
        "tar",
        "tar.gz",
        "tar.bz2",
        "tar.xz",
        "7z"
    };

    return supportedFormats.contains(suffix);
}

bool ArchiveManager::copy(const QUrl &sourcePath, const QUrl &destination)
{
    if (!sourcePath.isValid() || !destination.isValid()) {
        return false;
    }
    qDebug() << "copy:" << destination.toLocalFile();
    return QFile::copy(sourcePath.toLocalFile(), destination.toLocalFile() + "/" + sourcePath.fileName());
}

bool ArchiveManager::move(const QUrl &sourcePath, const QUrl &destination)
{
    if (!sourcePath.isValid() || !destination.isValid()) {
        return false;
    }
    return QFile::rename(sourcePath.toLocalFile(), destination.toLocalFile() + "/" + sourcePath.fileName());
}

QString ArchiveManager::iconName(const QString &fileName) const
{
    QString icon = QMimeDatabase().mimeTypeForFile(fileName).genericIconName();
    if (icon == "application-x-generic" || icon == "text-x-generic") {
        icon = "stock_document";
    }
    return icon;
}



void ArchiveManager::cleanDirectory(const QString &path)
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









