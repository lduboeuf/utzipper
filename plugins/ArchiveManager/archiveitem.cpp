#include "archiveitem.h"

ArchiveItem::ArchiveItem(const QString& name, bool isDir, const QString& fullPath)
    : mName(name), mIsDir(isDir), mFullPath(fullPath) {
}

QString ArchiveItem::name() const {
    return mName;
}

bool ArchiveItem::isDir() const {
    return mIsDir;
}

QString ArchiveItem::fullPath() const {
    return mFullPath;
}
