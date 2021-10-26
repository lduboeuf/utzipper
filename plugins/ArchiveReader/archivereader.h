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

class Item  {

public:
    Item(const QString& name, bool isDir, const QString& fullPath);

     QString name() const;
     bool isDir() const;
     QString fullPath() const;

private:
    QString mName;
    bool mIsDir;
    QString mFullPath;
};

class ArchiveReader: public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QString currentDir READ currentDir WRITE setCurrentDir NOTIFY currentDirChanged)
    Q_PROPERTY(QString archive READ archive WRITE setArchive NOTIFY archiveChanged)
    Q_PROPERTY(Errors error READ error NOTIFY errorChanged)
    //Q_PROPERTY(QList<Item> model READ items NOTIFY modelChanged)

public:
    ArchiveReader(QObject *parent = 0);
    ~ArchiveReader();

    enum ItemRoles {
            NameRole = Qt::UserRole + 1,
            IsDirRole,
            FullPathRole
        };

    enum Errors {
        NO_ERRORS,
        UNSUPPORTED_FILE_FORMAT,
        ERROR_READ,
        ERROR_UNKNOWN
    };
    Q_ENUM(Errors)

    // reimplemented from QAbstractListModel
    QHash<int, QByteArray> roleNames() const;
    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role) const;

    QString archive() const;
    void setArchive(const QString &path);
    QString currentDir() const;
    void setCurrentDir(const QString &currentDir);
    Errors error() const;

    Q_INVOKABLE QString extractFile(const QString &path);
    Q_INVOKABLE QVariantMap get(int index) const;
    //Q_INVOKABLE QQmlListProperty<Item> model(const QString &currentDir);
    //QQmlListProperty<Item> items();

Q_SIGNALS:
    void modelChanged();
    void currentDirChanged();
    void archiveChanged();
    void rowCountChanged();
    void errorChanged();

protected Q_SLOTS:
    void extract();
    void setError(const Errors& error);

private:
    QString mCurrentDir;
    QString mArchive;
    KArchive* mArchivePtr;
    Errors mError;
    QMap<QString, QList<Item>> mItems;
    QList<Item> mCurrentItems;
    QString mimeType( const QString &filePath ) const;
    KArchive* getKArchiveObject(const QString &filePath);


    void extractArchive(const KArchiveDirectory *dir, const QString &path);
};

#endif
