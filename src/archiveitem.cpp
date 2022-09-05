#include "archiveitem.h"

ArchiveItem::ArchiveItem(const QString& name, bool isDir, const QUrl& fullPath)
    : mName(name), mIsDir(isDir), mFullPath(fullPath) {
}

QString ArchiveItem::name() const {
    return mName;
}

bool ArchiveItem::isDir() const {
    return mIsDir;
}

QUrl ArchiveItem::fullPath() const {
    return mFullPath;
}

void ArchiveItem::setFullPath(const QUrl &fullPath)
{
    mFullPath = fullPath;
}

