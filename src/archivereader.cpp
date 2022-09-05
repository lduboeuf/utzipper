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

#include "archivereader.h"
#include "archiveitem.h"

ArchiveReader::ArchiveReader(QObject *parent) : QAbstractListModel(parent), mError(NO_ERRORS), mHasFiles(false) {
    connect(this, SIGNAL(archiveChanged()),this, SLOT(extract()));
    connect(this, SIGNAL(rowCountChanged()),this, SLOT(onRowCountChanged()));

    archiveMimeTypes.insert("zip", { "application/zip", "application/x-zip", "application/x-zip-compressed" });
    archiveMimeTypes.insert("tar", { "application/x-compressed-tar", "application/x-bzip-compressed-tar", "application/x-lzma-compressed-tar", "application/x-xz-compressed-tar", "application/x-gzip", "application/x-bzip", "application/x-lzma", "application/x-xz" });
    archiveMimeTypes.insert("7z", { "application/x-7z-compressed" });
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

QString ArchiveReader::mimeType( const QString &filePath ) const{
    QMimeType mimeType = QMimeDatabase().mimeTypeForFile(filePath);
    qDebug() << "mimeType:" << mimeType.iconName() << mimeType.genericIconName();
    return mimeType.name();
}

KArchive *ArchiveReader::getKArchiveObject(const QString &filePath)
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

    KArchive *mArchivePtr = getKArchiveObject(mArchive.toLocalFile());
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

void ArchiveReader::extractArchive(const KArchiveDirectory *dir, const QString &path)
{
    const QStringList entries = dir->entries();
    QStringList::const_iterator it = entries.constBegin();
    QList<ArchiveItem> archiveItems;
    for (; it != entries.end(); ++it)
    {
        const KArchiveEntry* entry = dir->entry((*it));
        ArchiveItem archiveItem(entry->name(), entry->isDirectory(), QUrl::fromLocalFile(path + entry->name()));
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
    mArchiveItems.insert(key, archiveItems);
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

