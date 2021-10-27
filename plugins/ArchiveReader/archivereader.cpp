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

#include "archivereader.h"

Item::Item(const QString& name, bool isDir, const QString& fullPath)
    : mName(name), mIsDir(isDir), mFullPath(fullPath) {
}

QString Item::name() const {
    return mName;
}

bool Item::isDir() const {
    return mIsDir;
}

QString Item::fullPath() const {
    return mFullPath;
}

ArchiveReader::ArchiveReader(QObject *parent) : QAbstractListModel(parent), mError(NO_ERRORS) {
    connect(this, SIGNAL(archiveChanged()),this, SLOT(extract()));
}

ArchiveReader::~ArchiveReader()
{
    if (mArchivePtr) {
        mArchivePtr->close();
    }
}

QString ArchiveReader::archive() const
{
    return mArchive;
}

void ArchiveReader::setArchive(const QString &path)
{
    if (mArchive == path) {
        return;
    }
    mArchive = path;
    Q_EMIT archiveChanged();
}

QString ArchiveReader::currentDir() const
{
    return mCurrentDir;
}

void ArchiveReader::setCurrentDir(const QString &currentDir)
{
    mCurrentDir = currentDir;
    Q_EMIT currentDirChanged();

    beginResetModel();
    mCurrentItems.clear();
    mCurrentItems = mItems.value(mCurrentDir);
    endResetModel();

    Q_EMIT rowCountChanged();
}

ArchiveReader::Errors ArchiveReader::error() const
{
    return mError;
}

QString ArchiveReader::extractFile(const QString &path)
{

    if (!mArchivePtr) {
        qWarning() << "Cannot open " << mArchive;
        Q_EMIT setError(ArchiveReader::ERROR_UNKNOWN);
        return "";
    }

    QString output = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    qDebug() << "tmp-dir:" << output;
    // Take the root folder from the archive and create a KArchiveDirectory object.
    // KArchiveDirectory represents a directory in a KArchive.
    const KArchiveDirectory *rootDir = mArchivePtr->directory();
    const KArchiveFile *localFile = rootDir->file(path);
    if (localFile) {
            bool localCopyTo = localFile->copyTo(output);
            qDebug() << localFile->name() << "copy to" << output + "/" + localFile->name();
            return output + "/" + localFile->name();
    }

    return "";
}

QVariantMap ArchiveReader::get(int i) const
{
    QVariantMap item;
    QHash<int, QByteArray> roles = roleNames();

    QModelIndex modelIndex = index(i, 0);
    if (modelIndex.isValid()) {
        Q_FOREACH(int role, roles.keys()) {
            QString roleName = QString::fromUtf8(roles.value(role));
            item.insert(roleName, data(modelIndex, role));
        }
    }
    return item;
}

QString ArchiveReader::mimeType( const QString &filePath ) const{
    return QMimeDatabase().mimeTypeForFile(filePath).name();
}

KArchive *ArchiveReader::getKArchiveObject(const QString &filePath)
{
    KArchive *kArch = nullptr;

    QString mime = mimeType(filePath);

    if (mime == "application/zip" ||
            mime == "application/x-zip" ||
            mime == "application/x-zip-compressed") {
        kArch = new KZip(filePath);
    } else if (mime == "application/x-compressed-tar" ||
               mime == "application/x-bzip-compressed-tar" ||
               mime == "application/x-lzma-compressed-tar" ||
               mime == "application/x-xz-compressed-tar" ||
               mime == "application/x-gzip" ||
               mime == "application/x-bzip" ||
               mime == "application/x-lzma" ||
               mime == "application/x-xz") {
        kArch = new KTar(filePath);
    }else if (mime.contains("application/x-7z-compressed")){
        kArch = new K7Zip(filePath);
    } else {
        qWarning() << "ERROR. COMPRESSED FILE TYPE UNKOWN " << filePath;
    }

    return kArch;
}

void ArchiveReader::extract()
{
    mArchivePtr = getKArchiveObject(mArchive);
    if (!mArchivePtr) {
        qWarning() << "Cannot open " << mArchive;
        setError(Errors::UNSUPPORTED_FILE_FORMAT);
        return;
    }
    // Open the archive
    if (!mArchivePtr->open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open " << mArchive;
        setError(Errors::ERROR_READ);
        return;
    }

    // Take the root folder from the archive and create a KArchiveDirectory object.
    // KArchiveDirectory represents a directory in a KArchive.
    const KArchiveDirectory *rootDir = mArchivePtr->directory();

    // We can extract all contents from a KArchiveDirectory to a destination.
    // recursive true will also extract subdirectories.
    extractArchive(rootDir, "");

    Q_EMIT modelChanged();
    setCurrentDir("");
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
    QList<Item> items;
    for (; it != entries.end(); ++it)
    {
        const KArchiveEntry* entry = dir->entry((*it));
        Item item(entry->name(), entry->isDirectory(), path + entry->name());
        items << item;

        if (entry->isDirectory()) {
            extractArchive((KArchiveDirectory *)entry, path+(*it)+'/');
        }
    }
    QString key(path);
    key.chop(1); //remove last "/"

    //put directory on top and sort by name
    std::sort(items.begin() , items.end(), [this]( const Item& test1 , const Item& test2 )->bool {
        if (test1.isDir() != test2.isDir()) {
            return test1.isDir();
        } else {
            return test1.name().compare(test2.name(), Qt::CaseInsensitive) < 0;
        }
    });

    mItems.insert(key, items);
}

QHash<int, QByteArray> ArchiveReader::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[IsDirRole] = "isDir";
    roles[FullPathRole] = "fullPath";
    return roles;
}

QVariant ArchiveReader::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= mCurrentItems.count())
        return QVariant();


    const Item &item = mCurrentItems[index.row()];
    if (role == NameRole)
        return QVariant::fromValue(item.name());
    else if (role == IsDirRole)
        return QVariant::fromValue(item.isDir());
    else if (role == FullPathRole)
        return QVariant::fromValue(item.fullPath());
    else
        return QVariant();
}

int ArchiveReader::rowCount(const QModelIndex & parent) const {
    Q_UNUSED(parent);
    return mCurrentItems.count();
}

