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

#ifndef ArchiveReader_H
#define ArchiveReader_H

#include <QAbstractListModel>
#include <QObject>

#include "archiveitem.h"
#include "archiveutils.h"

class ArchiveReader: public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QString currentDir READ currentDir WRITE setCurrentDir NOTIFY currentDirChanged)
    Q_PROPERTY(QUrl archive READ archive WRITE setArchive NOTIFY archiveChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString passphrase READ passphrase WRITE setPassphrase NOTIFY passphraseChanged)
    Q_PROPERTY(bool hasFiles READ hasFiles NOTIFY hasFilesChanged)
    Q_PROPERTY(bool requiresPassphrase READ requiresPassphrase NOTIFY requiresPassphraseChanged)
    Q_PROPERTY(bool encrypted READ encrypted NOTIFY encryptedChanged)
    Q_PROPERTY(Errors error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

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
        ERROR_PASSPHRASE_REQUIRED,
        ERROR_INVALID_PASSPHRASE,
        ERROR_ENCRYPTION_UNSUPPORTED,
        ERROR_READ,
        ERROR_WRITE,
        ERROR_UNKNOWN
    };
    Q_ENUM(Errors)

    // reimplemented from QAbstractListModel
    QHash<int, QByteArray> roleNames() const;
    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role) const;

    QUrl archive() const;
    void setArchive(const QUrl &path);
    QString name() const;
    QString passphrase() const;
    void setPassphrase(const QString &passphrase);
    bool hasFiles() const;
    bool requiresPassphrase() const;
    bool encrypted() const;
    QString currentDir() const;
    void setCurrentDir(const QString &currentDir);
    Errors error() const;
    QString errorMessage() const;

    Q_INVOKABLE void clear();
    Q_INVOKABLE bool hasData() const;
    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE void retry();

Q_SIGNALS:
    void modelChanged();
    void currentDirChanged();
    void archiveChanged();
    void rowCountChanged();
    void errorChanged();
    void errorMessageChanged();
    void hasFilesChanged();
    void nameChanged();
    void passphraseChanged();
    void requiresPassphraseChanged();
    void encryptedChanged();

protected Q_SLOTS:
    void extract();
    void onRowCountChanged();
    void setError(const Errors& error);

private:
    QString mCurrentDir;
    QUrl mArchive;
    QString mName;
    QString mPassphrase;
    QString mNewArchiveDir;
    bool mHasFiles;
    bool mRequiresPassphrase;
    bool mEncrypted;
    Errors mError;
    QString mErrorMessage;
    QMap<QString, QList<ArchiveItem>> mArchiveItems;
    QList<ArchiveItem> mCurrentArchiveItems;
    void addArchiveEntry(const QString &entryPath, bool isDir);
    void sortArchiveItems();
    void setErrorMessage(const QString &message);
    void setRequiresPassphrase(bool required);
    void setEncrypted(bool encrypted);

    void cleanDirectory(const QString &path);
};

#endif
