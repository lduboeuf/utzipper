/*
 * Copyright (C) 2021  Your FullName
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

#ifndef ArchiveReader_H
#define ArchiveReader_H

#include <QQmlListProperty>
#include <QAbstractListModel>
#include <QObject>
#include <KArchive>

#include "archiveitem.h"

class ArchiveManager: public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QString currentDir READ currentDir WRITE setCurrentDir NOTIFY currentDirChanged)
    Q_PROPERTY(QString archive READ archive WRITE setArchive NOTIFY archiveChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(bool hasFiles READ hasFiles NOTIFY hasFilesChanged)
    Q_PROPERTY(bool hasData READ hasData NOTIFY hasDataChanged)
    Q_PROPERTY(Errors error READ error NOTIFY errorChanged)

public:
    ArchiveManager(QObject *parent = 0);
    ~ArchiveManager();

    enum ItemRoles {
            NameRole = Qt::UserRole + 1,
            IsDirRole,
            FullPathRole
        };

    enum Errors {
        NO_ERRORS,
        UNSUPPORTED_FILE_FORMAT,
        ERROR_READ,
        ERROR_WRITE,
        ERROR_UNKNOWN
    };
    Q_ENUM(Errors)

    // reimplemented from QAbstractListModel
    QHash<int, QByteArray> roleNames() const;
    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role) const;

    QString archive() const;
    void setArchive(const QString &path);
    QString name() const;
    bool hasFiles() const;
    bool hasData() const;
    QString currentDir() const;
    void setCurrentDir(const QString &currentDir);
    Errors error() const;

    Q_INVOKABLE void clear();
    //Q_INVOKABLE bool hasData();
    Q_INVOKABLE QStringList extractFiles(const QStringList &files);
    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE bool isArchiveFile(const QString &path);
    Q_INVOKABLE void appendFile(const QString &filePath, const QString &parentFolder);
    Q_INVOKABLE void removeFile(const QString &name, const QString &parentFolder);
    Q_INVOKABLE void appendFolder(const QString &name, const QString &parentFolder);
    Q_INVOKABLE void removeFolder(const QString &name, const QString &parentFolder);
    Q_INVOKABLE QString save(const QString &archiveName, const QString &suffix);


Q_SIGNALS:
    void modelChanged();
    void currentDirChanged();
    void archiveChanged();
    void rowCountChanged();
    void errorChanged();
    void hasFilesChanged();
    void nameChanged();
    void hasDataChanged();

protected Q_SLOTS:
    void extract();
    void onRowCountChanged();
    void setError(const Errors& error);

private:
    QString mCurrentDir;
    QString mArchive;
    QString mName;
    bool mHasFiles;
    //KArchive* mArchivePtr;
    Errors mError;
    QMap<QString, QList<ArchiveItem>> mArchiveItems;
    QMap<QString, QStringList> archiveMimeTypes;
    QList<ArchiveItem> mCurrentArchiveItems;
    QString mimeType( const QString &filePath ) const;
    KArchive* getKArchiveObject(const QString &filePath);


    void extractArchive(const KArchiveDirectory *dir, const QString &path);
};

#endif
