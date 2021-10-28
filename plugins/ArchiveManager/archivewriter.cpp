#include "archivewriter.h"
#include <KZip>
#include <QFileInfo>
#include <QDebug>
#include <QStandardPaths>

ArchiveWriter::ArchiveWriter(QObject *parent) : QObject(parent)
{

}

ArchiveWriter::~ArchiveWriter()
{
    if (mArchivePtr) {
        mArchivePtr->close();
    }
}

void ArchiveWriter::appendFile(const QString &filePath, const QString &parentFolder)
{
    if (mArchivePtr == nullptr) {
        QString output = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        mArchivePtr = new KZip(output + "/archive.zip");
        if (!mArchivePtr->open(QIODevice::WriteOnly)) {
            qDebug() << "could not open archive";
        }
    }

    QFileInfo info(filePath);
    qDebug() << "file:" << info.absolutePath();
    mArchivePtr->addLocalFile(filePath, info.fileName());

    const KArchiveDirectory *dir = mArchivePtr->directory();
    const QStringList entries = dir->entries();
    QStringList::const_iterator it = entries.constBegin();
    for (; it != entries.end(); ++it)
    {
        const KArchiveEntry* entry = dir->entry((*it));
        qDebug() << entry->name();
    }
}

void ArchiveWriter::appendFolder(const QString &name, const QString &parentFolder)
{
   Q_UNUSED(name);
   Q_UNUSED(parentFolder);
}

QString ArchiveWriter::save()
{
    if (mArchivePtr) {
        mArchivePtr->close();
        return mArchivePtr->fileName();
    }

    return "";
}
