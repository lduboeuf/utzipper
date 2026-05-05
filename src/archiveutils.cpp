#include "archiveutils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QStringList>
#include <archive.h>
#include <archive_entry.h>

namespace {

archive *createArchiveReader()
{
    archive *reader = archive_read_new();
    archive_read_support_filter_all(reader);
    archive_read_support_format_all(reader);
    return reader;
}

QString completeSuffixLower(const QString &filePath)
{
    return QFileInfo(filePath).completeSuffix().toLower();
}

QString archiveExtensionKey(const QString &filePath)
{
    const QString suffix = completeSuffixLower(filePath);
    if (suffix == "tar.gz" || suffix == "tar.bz2" || suffix == "tar.xz" || suffix == "tar.zst") {
        return suffix;
    }

    const QString fileName = QFileInfo(filePath).fileName().toLower();
    if (fileName.endsWith(".tgz")) {
        return "tgz";
    }
    if (fileName.endsWith(".tbz2") || fileName.endsWith(".tbz")) {
        return "tbz2";
    }
    if (fileName.endsWith(".txz")) {
        return "txz";
    }

    return QFileInfo(filePath).suffix().toLower();
}

bool hasSupportedExtension(const QString &filePath)
{
    static const QStringList supportedExtensions = {
        "7z",
        "ar",
        "click",
        "deb",
        "gz",
        "rar",
        "tar",
        "tar.bz2",
        "tar.gz",
        "tar.xz",
        "tar.zst",
        "tbz",
        "tbz2",
        "tgz",
        "txz",
        "xz",
        "zip"
    };

    return supportedExtensions.contains(archiveExtensionKey(filePath));
}

bool isPackageContainer(const QString &filePath)
{
    const QString extension = archiveExtensionKey(filePath);
    return extension == "deb" || extension == "click";
}

bool hasSupportedMime(const QString &mime)
{
    static const QStringList supportedMimes = {
        "application/gzip",
        "application/vnd.debian.binary-package",
        "application/vnd.rar",
        "application/x-7z-compressed",
        "application/x-archive",
        "application/x-ar",
        "application/x-bzip",
        "application/x-bzip-compressed-tar",
        "application/x-compressed-tar",
        "application/x-gzip",
        "application/x-lzma",
        "application/x-lzma-compressed-tar",
        "application/x-rar",
        "application/x-rar-compressed",
        "application/x-tar",
        "application/x-xz",
        "application/x-xz-compressed-tar",
        "application/zip"
    };

    return supportedMimes.contains(mime);
}

bool readCurrentEntryData(archive *reader, QByteArray &output, QString *errorMessage)
{
    output.clear();
    char buffer[8192];
    la_ssize_t bytesRead = 0;
    while ((bytesRead = archive_read_data(reader, buffer, sizeof(buffer))) > 0) {
        output.append(buffer, static_cast<int>(bytesRead));
    }

    if (bytesRead < 0) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8(archive_error_string(reader));
        }
        return false;
    }

    return true;
}

bool openPackageDataArchive(const QString &filePath, ArchiveReadHandle &handle, QString *errorMessage)
{
    archive *outerReader = createArchiveReader();
    const QByteArray encodedPath = QFile::encodeName(filePath);
    if (archive_read_open_filename(outerReader, encodedPath.constData(), 10240) != ARCHIVE_OK) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8(archive_error_string(outerReader));
        }
        archive_read_free(outerReader);
        return false;
    }

    archive_entry *entry = nullptr;
    bool foundDataArchive = false;
    while (archive_read_next_header(outerReader, &entry) == ARCHIVE_OK) {
        const char *rawPath = archive_entry_pathname(entry);
        QString entryPath = rawPath ? QString::fromUtf8(rawPath).toLower() : QString();
        while (entryPath.endsWith('/')) {
            entryPath.chop(1);
        }

        if (entryPath.startsWith("data.tar")) {
            foundDataArchive = readCurrentEntryData(outerReader, handle.buffer, errorMessage);
            break;
        }

        archive_read_data_skip(outerReader);
    }

    archive_read_close(outerReader);
    archive_read_free(outerReader);

    if (!foundDataArchive) {
        if (errorMessage && errorMessage->isEmpty()) {
            *errorMessage = QStringLiteral("Missing data.tar member in package");
        }
        handle.buffer.clear();
        return false;
    }

    handle.reader = createArchiveReader();
    if (archive_read_open_memory(handle.reader, handle.buffer.constData(), static_cast<size_t>(handle.buffer.size())) != ARCHIVE_OK) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8(archive_error_string(handle.reader));
        }
        archive_read_free(handle.reader);
        handle.reader = nullptr;
        handle.buffer.clear();
        return false;
    }

    return true;
}

} // namespace

QString archiveMimeTypeForFile(const QString &filePath)
{
    return QMimeDatabase().mimeTypeForFile(filePath).name();
}

bool isSupportedArchiveFile(const QString &filePath)
{
    return hasSupportedExtension(filePath) || hasSupportedMime(archiveMimeTypeForFile(filePath));
}

bool openArchiveForReading(const QString &filePath, ArchiveReadHandle &handle, QString *errorMessage)
{
    closeArchiveReader(handle);

    if (isPackageContainer(filePath)) {
        return openPackageDataArchive(filePath, handle, errorMessage);
    }

    handle.reader = createArchiveReader();
    const QByteArray encodedPath = QFile::encodeName(filePath);
    if (archive_read_open_filename(handle.reader, encodedPath.constData(), 10240) != ARCHIVE_OK) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8(archive_error_string(handle.reader));
        }
        archive_read_free(handle.reader);
        handle.reader = nullptr;
        return false;
    }

    return true;
}

void closeArchiveReader(ArchiveReadHandle &handle)
{
    if (handle.reader) {
        archive_read_close(handle.reader);
        archive_read_free(handle.reader);
        handle.reader = nullptr;
    }
    handle.buffer.clear();
}

