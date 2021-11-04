#ifndef ARCHIVEWRITER_H
#define ARCHIVEWRITER_H

#include <QObject>
#include <KArchive>

class ArchiveWriter : public QObject
{
   Q_OBJECT
public:
    ArchiveWriter(QObject *parent = 0);
    ~ArchiveWriter();

    Q_INVOKABLE void appendFile(const QString &filePath, const QString &parentFolder);
    Q_INVOKABLE void appendFolder(const QString &name, const QString &parentFolder);
    Q_INVOKABLE QString save();
    //TODO CLEAN

private:
    KArchive* mArchivePtr = nullptr;
};
#endif // ARCHIVEWRITER_H
