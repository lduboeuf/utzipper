#ifndef ARCHIVEUTILS_H
#define ARCHIVEUTILS_H

#include <QByteArray>
#include <QString>

struct archive;

struct ArchiveReadHandle {
    archive *reader;
    QByteArray buffer;

    ArchiveReadHandle()
        : reader(nullptr)
    {
    }
};

QString archiveMimeTypeForFile(const QString &filePath);
bool isSupportedArchiveFile(const QString &filePath);
bool openArchiveForReading(const QString &filePath, ArchiveReadHandle &handle, QString *errorMessage = nullptr);
void closeArchiveReader(ArchiveReadHandle &handle);

#endif // ARCHIVEUTILS_H

