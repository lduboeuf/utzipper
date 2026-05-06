#ifndef ARCHIVEUTILS_H
#define ARCHIVEUTILS_H

#include <QByteArray>
#include <QString>

struct archive;

enum class ArchiveOpenError {
    NoError,
    UnsupportedFormat,
    PasswordRequired,
    InvalidPassword,
    EncryptionUnsupported,
    ReadError
};

struct ArchiveReadHandle {
    archive *reader;
    QByteArray buffer;
    bool encrypted;

    ArchiveReadHandle()
        : reader(nullptr)
        , encrypted(false)
    {
    }
};

QString archiveMimeTypeForFile(const QString &filePath);
bool isSupportedArchiveFile(const QString &filePath);
QString archiveErrorString(archive *handle);
ArchiveOpenError archiveReadError(archive *reader, const QString &passphrase = QString(), const QString &fallbackMessage = QString());
bool openArchiveForReading(const QString &filePath, ArchiveReadHandle &handle, const QString &passphrase = QString(), ArchiveOpenError *openError = nullptr, QString *errorMessage = nullptr);
void closeArchiveReader(ArchiveReadHandle &handle);

#endif // ARCHIVEUTILS_H

