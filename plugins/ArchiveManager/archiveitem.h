#ifndef ARCHIVEITEM_H
#define ARCHIVEITEM_H

#include <QString>

class ArchiveItem {

public:
    ArchiveItem(const QString& name, bool isDir, const QString& fullPath);

     QString name() const;
     bool isDir() const;
     QString fullPath() const;
     void setFullPath(const QString &fullPath);

private:
    QString mName;
    bool mIsDir;
    QString mFullPath;
};

#endif // ARCHIVEITEM_H
