/*
 * Copyright (C) 2021  Your FullName
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

ArchiveManager::ArchiveManager(QObject *parent) : QAbstractListModel(parent), mError(NO_ERRORS), mHasFiles(false) {
    connect(this, SIGNAL(archiveChanged()),this, SLOT(extract()));
    connect(this, SIGNAL(rowCountChanged()),this, SLOT(onRowCountChanged()));

    archiveMimeTypes.insert("zip", { "application/zip", "application/x-zip", "application/x-zip-compressed" });
    archiveMimeTypes.insert("tar", { "application/x-compressed-tar", "application/x-bzip-compressed-tar", "application/x-lzma-compressed-tar", "application/x-xz-compressed-tar", "application/x-gzip", "application/x-bzip", "application/x-lzma", "application/x-xz" });
    archiveMimeTypes.insert("7z", { "application/x-7z-compressed" });
}

ArchiveManager::~ArchiveManager()
{
//    if (mArchivePtr && mArchivePtr->isOpen()) {
//        mArchivePtr->close();
//    }
}

QString ArchiveManager::archive() const
{
    return mArchive;
}

void ArchiveManager::setArchive(const QString &path)
{
    if (mArchive == path) {
        return;
    }
    mArchive = path;
    Q_EMIT archiveChanged();
}

QString ArchiveManager::name() const
{
    return mName;
}

bool ArchiveManager::hasFiles() const
{
    return mHasFiles;
}

QString ArchiveManager::currentDir() const
{
    return mCurrentDir;
}

void ArchiveManager::setCurrentDir(const QString &currentDir)
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

ArchiveManager::Errors ArchiveManager::error() const
{
    return mError;
}

void ArchiveManager::clear()
{
    mArchive = "";
    setError(Errors::NO_ERRORS);
    beginResetModel();
    mCurrentArchiveItems.clear();
    mArchiveItems.clear();
    endResetModel();

    //clean up temp dir
    QString output = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir dir(output);
    dir.setFilter( QDir::NoDotAndDotDot | QDir::Files );
    foreach( QString dirItem, dir.entryList() )
        dir.remove( dirItem );

    dir.setFilter( QDir::NoDotAndDotDot | QDir::Dirs );
    foreach( QString dirItem, dir.entryList() )
    {
        QDir subDir( dir.absoluteFilePath( dirItem ) );
        subDir.removeRecursively();
    }

    Q_EMIT rowCountChanged();
    Q_EMIT hasDataChanged();
}

bool ArchiveManager::hasData() const
{
    return mArchiveItems.count() > 0;
}

QStringList ArchiveManager::extractFiles(const QStringList &files)
{
    QStringList outFiles;
    KArchive *mArchivePtr = getKArchiveObject(mArchive);
    if (!mArchivePtr) {
        return outFiles;
    }
    //TODO ensure clean dir
    QString output = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    qDebug() << "tmp-dir:" << output;
    const KArchiveDirectory *rootDir = mArchivePtr->directory();
    foreach(QString path, files)
    {
        const KArchiveFile *localFile = rootDir->file(path);
        if (localFile) {
            bool localCopyTo = localFile->copyTo(output);
            qDebug() << localFile->name() << "copy to" << output + "/" + localFile->name();
            outFiles <<  output + "/" + localFile->name();
        }
    }
    mArchivePtr->close();

    return outFiles;
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

void ArchiveManager::appendFile(const QString &filePath, const QString &parentFolder)
{

    QFileInfo file(filePath);

    bool exist = mArchiveItems.contains(parentFolder);
    if (!exist) {
        mArchiveItems.insert(parentFolder, QList<ArchiveItem>());
    }

    ArchiveItem item(file.fileName(), false, file.absoluteFilePath());

    mArchiveItems[parentFolder] << item;
    setCurrentDir(parentFolder);
    Q_EMIT hasDataChanged();
}

void ArchiveManager::removeFile(const QString &name, const QString &parentFolder)
{
    bool exist = mArchiveItems.contains(parentFolder);
    if (exist) {
        QMutableListIterator<ArchiveItem> iter(mArchiveItems[parentFolder]);
        while (iter.hasNext()) {
            if (iter.next().name() == name) {
                iter.remove();
            }
        }
        setCurrentDir(parentFolder);
    }
}

void ArchiveManager::appendFolder(const QString &name, const QString &parentFolder)
{
    bool exist = mArchiveItems.contains(name);
    if (!exist) {
        const QString key = parentFolder.isEmpty() ? name : name + "/" + parentFolder;
        mArchiveItems.insert(key, QList<ArchiveItem>());
        ArchiveItem item(name, true, key);
        mArchiveItems[parentFolder] << item;
        setCurrentDir(parentFolder);
    }
}

void ArchiveManager::removeFolder(const QString &name, const QString &parentFolder)
{
    bool exist = mArchiveItems.contains(parentFolder);
    if (exist) {
        QMutableListIterator<ArchiveItem> iter(mArchiveItems[parentFolder]);
        while (iter.hasNext()) {
            ArchiveItem item = iter.next();
            if (item.name() == name && item.isDir()) {
                iter.remove();
            }
        }
        const QString key = parentFolder.isEmpty() ? name : name + "/" + parentFolder;
        mArchiveItems.remove(key);
        setCurrentDir(parentFolder);
    }
}

QString ArchiveManager::save(const QString &archiveName, const QString &suffix)
{
    QString tmpPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString output = tmpPath + "/" + archiveName + "." + suffix;

    KArchive* mArchivePtr;
    if (suffix == "zip") {
        mArchivePtr = new KZip(output);
    } else if (suffix == "tar") {
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

    QMapIterator<QString, QList<ArchiveItem>> iter(mArchiveItems);
    while (iter.hasNext()) {
        iter.next();

        foreach (ArchiveItem item, iter.value()) {
            if (item.isDir()) {
                mArchivePtr->addLocalDirectory(item.fullPath(), item.name());
            } else {
                mArchivePtr->addLocalFile(item.fullPath(), item.name());
            }
        }
    }
    mArchivePtr->close();
    qDebug() << "archive copied to:" << output;

    return output;
}

QString ArchiveManager::mimeType( const QString &filePath ) const{
    return QMimeDatabase().mimeTypeForFile(filePath).name();
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

    mName = info.fileName();
    Q_EMIT nameChanged();

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

void ArchiveManager::extract()
{

    setError(Errors::NO_ERRORS);

    KArchive *mArchivePtr = getKArchiveObject(mArchive);
    if (!mArchivePtr) {
        return;
    }

    // Take the root folder from the archive and create a KArchiveDirectory object.
    // KArchiveDirectory represents a directory in a KArchive.
    const KArchiveDirectory *rootDir = mArchivePtr->directory();

    // We can extract all contents from a KArchiveDirectory to a destination.
    // recursive true will also extract subdirectories.
    extractArchive(rootDir, "");

    mArchivePtr->close();
    Q_EMIT hasDataChanged();
    Q_EMIT modelChanged();
    setCurrentDir("");
}

void ArchiveManager::onRowCountChanged()
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

void ArchiveManager::setError(const ArchiveManager::Errors &error)
{
    mError = error;
    Q_EMIT errorChanged();
}

void ArchiveManager::extractArchive(const KArchiveDirectory *dir, const QString &path)
{

    const QStringList entries = dir->entries();
    QStringList::const_iterator it = entries.constBegin();
    QList<ArchiveItem> archiveItems;
    for (; it != entries.end(); ++it)
    {
        const KArchiveEntry* entry = dir->entry((*it));
        ArchiveItem archiveItem(entry->name(), entry->isDirectory(), path + entry->name());
        archiveItems << archiveItem;

        if (entry->isDirectory()) {
            extractArchive((KArchiveDirectory *)entry, path+(*it)+'/');
        }
    }
    QString key(path);
    key.chop(1); //remove last "/"

    //put directory on top and sort by name
    std::sort(archiveItems.begin() , archiveItems.end(), [this]( const ArchiveItem& test1 , const ArchiveItem& test2 )->bool {
        if (test1.isDir() != test2.isDir()) {
            return test1.isDir();
        } else {
            return test1.name().compare(test2.name(), Qt::CaseInsensitive) < 0;
        }
    });
    qDebug() << "key:" << key;
    mArchiveItems.insert(key, archiveItems);
}

QVariantMap ArchiveManager::get(int i) const
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

QHash<int, QByteArray> ArchiveManager::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[IsDirRole] = "isDir";
    roles[FullPathRole] = "fullPath";
    return roles;
}

QVariant ArchiveManager::data(const QModelIndex & index, int role) const {
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

int ArchiveManager::rowCount(const QModelIndex & parent) const {
    Q_UNUSED(parent);
    return mCurrentArchiveItems.count();
}

