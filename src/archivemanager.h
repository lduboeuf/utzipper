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

#ifndef ArchiveManager_H
#define ArchiveManager_H

#include <QQmlListProperty>
#include <QAbstractListModel>
#include <QObject>

#include "archiveitem.h"
#include "archiveutils.h"

class ArchiveManager: public QObject {
    Q_OBJECT

    Q_PROPERTY(QUrl currentDir READ currentDir WRITE setCurrentDir NOTIFY currentDirChanged)
    Q_PROPERTY(QString currentName READ currentName NOTIFY currentNameChanged)
    Q_PROPERTY(Errors error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QUrl tempDir READ tempDir NOTIFY tempDirChanged)
    Q_PROPERTY(QUrl newArchiveDir READ newArchiveDir NOTIFY newArchiveDirChanged)

public:
    ArchiveManager(QObject *parent = 0);
    ~ArchiveManager();

    enum Errors {
        NO_ERRORS,
        UNSUPPORTED_FILE_FORMAT,
        ERROR_PASSPHRASE_REQUIRED,
        ERROR_INVALID_PASSPHRASE,
        ERROR_ENCRYPTION_UNSUPPORTED,
        ERROR_READ,
        ERROR_WRITE,
        ERROR_UNKNOWN
    };
    Q_ENUM(Errors)

    bool hasFiles() const;
    QUrl currentDir() const;
    void setCurrentDir(const QUrl &currentDir);
    QString currentName() const;
    QUrl tempDir() const;
    void setTempDir(const QUrl &path);
    QUrl newArchiveDir() const;
    void setNewArchiveDir(const QUrl &path);
    Errors error() const;
    QString errorMessage() const;

    Q_INVOKABLE void clear();
    Q_INVOKABLE QList<QUrl> extractFiles(const QUrl &archive, const QList<QUrl> &files, const QString &passphrase = QString());
    Q_INVOKABLE void extractTo(const QUrl &archive, const QUrl &path, const QString &passphrase = QString());
    Q_INVOKABLE bool isArchiveFile(const QUrl &path);
    Q_INVOKABLE bool removeFile(const QUrl &file);
    Q_INVOKABLE bool appendFolder(const QString &name, const QUrl &dir);
    Q_INVOKABLE bool removeFolder(const QUrl &folder);
    Q_INVOKABLE QUrl save(const QString &archiveName, const QString &suffix, const QString &passphrase = QString());
    Q_INVOKABLE bool isWriteFormatSupported(const QString &suffix) const;
    Q_INVOKABLE bool isEncryptionSupported(const QString &suffix) const;
    Q_INVOKABLE bool copy(const QUrl &sourcePath, const QUrl &destination);
    Q_INVOKABLE bool move(const QUrl &sourcePath, const QUrl &destination);
    Q_INVOKABLE QString iconName(const QString &fileName) const;


Q_SIGNALS:
    void currentDirChanged();
    void currentNameChanged();
    void errorChanged();
    void errorMessageChanged();
    void newArchiveDirChanged();
    void tempDirChanged();

protected Q_SLOTS:
    void setError(const Errors& error);

private:
    QUrl mCurrentDir;
    QUrl mNewArchiveDir;
    QUrl mTempDir;

    Errors mError;
    QString mErrorMessage;

    void cleanDirectory(const QString &path);
    void setErrorMessage(const QString &message);
};

#endif
