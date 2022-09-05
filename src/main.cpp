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

#include <QGuiApplication>
#include <QCoreApplication>
#include <QUrl>
#include <QDir>
#include <QQuickView>
#include <QStandardPaths>
#include "archivereader.h"
#include "archivemanager.h"

int main(int argc, char *argv[])
{
    QGuiApplication *app = new QGuiApplication(argc, (char**)argv);
    app->setApplicationName("utzip.lduboeuf");

    //make sure we have the new Archive Path
    QString output = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/newArchive";
    bool ok = QDir().mkpath(output);
    if (ok) {
        qDebug() << output << " created or already exist";
    }

    qmlRegisterType<ArchiveReader>("utzip.private",1, 0, "ArchiveReader");
    qmlRegisterSingletonType<ArchiveManager>("utzip.private", 1, 0, "ArchiveManager", [](QQmlEngine*, QJSEngine*) -> QObject* { return new ArchiveManager; });


    QQuickView *view = new QQuickView();
    view->setSource(QUrl("qrc:/Main.qml"));
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->show();

    return app->exec();
}
