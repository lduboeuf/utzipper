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
#include <KArchive>

#include "archiveitem.h"

class ArchiveManager: public QObject {
    Q_OBJECT

    Q_PROPERTY(QString currentDir READ currentDir WRITE setCurrentDir NOTIFY currentDirChanged)
    Q_PROPERTY(Errors error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString tempDir READ tempDir NOTIFY tempDirChanged)
    Q_PROPERTY(QString newArchiveDir READ newArchiveDir NOTIFY newArchiveDirChanged)

public:
    ArchiveManager(QObject *parent = 0);
    ~ArchiveManager();

    enum Errors {
        NO_ERRORS,
        UNSUPPORTED_FILE_FORMAT,
        ERROR_READ,
        ERROR_WRITE,
        ERROR_UNKNOWN
    };
    Q_ENUM(Errors)

    bool hasFiles() const;
    QString currentDir() const;
    void setCurrentDir(const QString &currentDir);
    QString tempDir() const;
    void setTempDir(const QString &path);
    QString newArchiveDir() const;
    void setNewArchiveDir(const QString &path);
    Errors error() const;

    Q_INVOKABLE void clear();
    Q_INVOKABLE QStringList extractFiles(const QString &archive, const QStringList &files);
    Q_INVOKABLE void extractTo(const QString &archive, const QString &path);
    Q_INVOKABLE bool isArchiveFile(const QString &path);
    Q_INVOKABLE bool removeFile(const QString &name, const QString &parentFolder);
    Q_INVOKABLE bool appendFolder(const QString &name, const QString &parentFolder);
    Q_INVOKABLE bool removeFolder(const QString &name, const QString &parentFolder);
    Q_INVOKABLE QString save(const QString &archiveName, const QString &suffix);
    Q_INVOKABLE bool copy(const QUrl &sourcePath, const QUrl &destination);
    Q_INVOKABLE bool move(const QUrl &sourcePath, const QUrl &destination);
    Q_INVOKABLE QString iconName(const QString &fileName) const;


Q_SIGNALS:
    void currentDirChanged();
    void errorChanged();
    void newArchiveDirChanged();
    void tempDirChanged();

protected Q_SLOTS:
    void setError(const Errors& error);

private:
    QString mCurrentDir;
    QString mNewArchiveDir;
    QString mTempDir;

    Errors mError;
    QMap<QString, QStringList> archiveMimeTypes;
    QString mimeType( const QString &filePath ) const;
    KArchive* getKArchiveObject(const QString &filePath);

    void cleanDirectory(const QString &path);
};

#endif
