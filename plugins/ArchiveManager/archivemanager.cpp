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
#include <KAr>
#include <KTar>
#include <KZip>
#include <K7Zip>
#include <KAr>

#include "archivemanager.h"
#include "archiveitem.h"

ArchiveManager::ArchiveManager(QObject *parent) : QObject(parent), mError(NO_ERRORS) {

    archiveMimeTypes.insert("zip", { "application/zip", "application/x-zip", "application/x-zip-compressed" });
    archiveMimeTypes.insert("tar", { "application/x-compressed-tar", "application/x-bzip-compressed-tar", "application/x-lzma-compressed-tar", "application/x-xz-compressed-tar", "application/x-gzip", "application/x-bzip", "application/x-lzma", "application/x-xz" });
    archiveMimeTypes.insert("7z", { "application/x-7z-compressed" });

    // working directory for new archives
    QString output = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/newArchive";
    QDir().mkdir(output);
    setNewArchiveDir(output);

    // temp dir
    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    setTempDir(tmpDir);
}

ArchiveManager::~ArchiveManager()
{
}

QString ArchiveManager::currentDir() const
{
    return mCurrentDir;
}

void ArchiveManager::setCurrentDir(const QString &currentDir)
{
    if (mCurrentDir == currentDir) {
        return;
    }
    qDebug() << "currentDir:" << currentDir;
    mCurrentDir = currentDir;
    Q_EMIT currentDirChanged();
}

QString ArchiveManager::tempDir() const
{
    return mTempDir;
}

void ArchiveManager::setTempDir(const QString &path)
{
    if (mTempDir != path) {
        mTempDir = path;
        qDebug() << "tempDir" << path;
        Q_EMIT tempDirChanged();
    }
}

QString ArchiveManager::newArchiveDir() const
{
    return mNewArchiveDir;
}

void ArchiveManager::setNewArchiveDir(const QString &path)
{
    if (mNewArchiveDir != path) {
        mNewArchiveDir = path;
        qDebug() << "tempDir" << path;
        Q_EMIT newArchiveDirChanged();
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
    cleanDirectory(mNewArchiveDir);

    //clean tmp dir
    cleanDirectory(mTempDir);

}


QStringList ArchiveManager::extractFiles(const QString &archive, const QStringList &files)
{
    QStringList outFiles;
    KArchive *mArchivePtr = getKArchiveObject(archive);
    if (!mArchivePtr) {
        return outFiles;
    }

    QString output = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    const KArchiveDirectory *rootDir = mArchivePtr->directory();
    foreach(QString path, files)
    {
        const KArchiveFile *localFile = rootDir->file(path);
        if (localFile) {
            bool localCopyTo = localFile->copyTo(output);
            outFiles <<  output + "/" + localFile->name();
        }
    }
    mArchivePtr->close();

    return outFiles;
}

/**
 * Extract the archive in the path folder
 */
void ArchiveManager::extractTo(const QString &archive, const QString &path)
{
    KArchive *mArchivePtr = getKArchiveObject(archive);
    if (!mArchivePtr) {
        return;
    }

    const KArchiveDirectory *rootDir = mArchivePtr->directory();
    rootDir->copyTo(path, true);
    mArchivePtr->close();

    setCurrentDir("");
}

bool ArchiveManager::isArchiveFile(const QString &path)
{
    QString mime = mimeType(path);
    QList<QStringList> valuesList = archiveMimeTypes.values();
    foreach(QStringList value, valuesList)
    {
        if(value.contains(mime))
        {
            return true;
        }
    }
    return false;
}

bool ArchiveManager::removeFile(const QString &name, const QString &parentFolder)
{

    QFile fileToRemove(parentFolder + "/" + name);
    return fileToRemove.remove();
}

bool ArchiveManager::appendFolder(const QString &name, const QString &parentFolder)
{

        const QString key = parentFolder.isEmpty() ? name : parentFolder + "/" + name;
        qDebug() << "new folder:" << mNewArchiveDir + "/" + key;
        return QDir().mkdir(mNewArchiveDir + "/" + key);
}

bool ArchiveManager::removeFolder(const QString &name, const QString &parentFolder)
{
    QDir folder(parentFolder + "/" + name);
    return folder.removeRecursively();
}

QString ArchiveManager::save(const QString &archiveName, const QString &suffix)
{
    QString tmpPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString output = tmpPath + "/" + archiveName + "." + suffix;

    KArchive* mArchivePtr;
    if (suffix == "zip") {
        mArchivePtr = new KZip(output);
    } else if (suffix == "tar" || suffix == "tar.gz" || suffix == "tar.bz2" || suffix == "tar.xz") {
        mArchivePtr = new KTar(output);
    }else if (suffix == "7z"){
        mArchivePtr = new K7Zip(output);
    } else {
        qWarning() << "ERROR. COMPRESSED FILE TYPE UNKOWN " << output;
        setError(Errors::UNSUPPORTED_FILE_FORMAT);
        return "";
    }

    if (!mArchivePtr->open(QIODevice::WriteOnly)) {
        setError(Errors::ERROR_WRITE);
        qWarning() << "could not open archive for writing";
        return "";
    }

    QDir dir(mNewArchiveDir);
    dir.setFilter( QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot );
    foreach(const QFileInfo dirItem, dir.entryInfoList() ) {
        if (dirItem.isDir()) {
            mArchivePtr->addLocalDirectory(dirItem.absoluteFilePath(), dirItem.fileName());
        } else {
            mArchivePtr->addLocalFile(dirItem.absoluteFilePath(),dirItem.fileName());
        }
    }

    mArchivePtr->close();
    qDebug() << "archive copied to:" << output;

    return output;
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

QString ArchiveManager::mimeType( const QString &filePath ) const{
    QMimeType mimeType = QMimeDatabase().mimeTypeForFile(filePath);
    return mimeType.name();
}

KArchive *ArchiveManager::getKArchiveObject(const QString &filePath)
{
    KArchive *kArch = nullptr;

    QFileInfo info(filePath);
    if (!info.isReadable()) {
        qWarning() << "Cannot read " << filePath;
        setError(Errors::ERROR_READ);
        return nullptr;
    }

    QString mime = mimeType(filePath);

    if (archiveMimeTypes["zip"].contains(mime)) {
        kArch = new KZip(filePath);
    } else if (archiveMimeTypes["tar"].contains(mime)) {
        kArch = new KTar(filePath);
    }else if (archiveMimeTypes["7z"].contains(mime)){
        kArch = new K7Zip(filePath);
    } else {
        qWarning() << "ERROR. COMPRESSED FILE TYPE UNKOWN " << filePath;
    }

    if (!kArch) {
        qWarning() << "Cannot open " << filePath;
        setError(Errors::UNSUPPORTED_FILE_FORMAT);
        return nullptr;
    }
    // Open the archive
    if (!kArch->open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open " << filePath;
        setError(Errors::ERROR_READ);
        return nullptr;
    }

    return kArch;
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









