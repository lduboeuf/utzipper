#ifndef ARCHIVEITEM_H
#define ARCHIVEITEM_H

#include <QString>
#include <QUrl>

class ArchiveItem {

public:
    ArchiveItem(const QString& name, bool isDir, const QUrl& fullPath);

    QString name() const;
    bool isDir() const;
    QUrl fullPath() const;
    void setFullPath(const QUrl &fullPath);

private:
    QString mName;
    bool mIsDir;
    QUrl mFullPath;
};

#endif // ARCHIVEITEM_H
