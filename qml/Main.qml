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

import QtQuick 2.7
import Ubuntu.Components 1.3
import QtQuick.Layouts 1.3
import Ubuntu.Content 1.3
import Qt.labs.settings 1.0

import ArchiveManager 1.0

MainView {
    id: root
    objectName: 'mainView'
    applicationName: 'utzip.lduboeuf'
    automaticOrientation: true

    width: units.gu(45)
    height: units.gu(75)

    property var activeTransfer

    function cleanup() {
        if (activeTransfer) {
            console.log('cleanup transfer');
            activeTransfer.finalize();
        }
        console.log('cleanup archive');
        archiveManager.clear();
    }

    ArchiveManager {
        id: archiveManager
    }

    Page {
        id: importPicker
        visible: false
        header: PageHeader {
            id: importPickerHeader
            title: i18n.tr("Choose from")
        }

        ContentPeerPicker {
            //visible: parent.visible
            anchors.top: importPickerHeader.bottom
            anchors.topMargin: units.gu(1)
            handler: ContentHandler.Source
            contentType: ContentType.Documents
            showTitle: false

            onPeerSelected: {
                peer.selectionType = ContentTransfer.Multiple;
                root.activeTransfer = peer.request();
                pageStack.pop()
            }

            onCancelPressed: pageStack.pop();
        }
    }

    ContentTransferHint {
        anchors.fill: parent
        activeTransfer: root.activeTransfer
    }

    Page {
        id: home
        anchors.fill: parent
        visible: false

        header: PageHeader {
            id: header
            title: i18n.tr('UT zipper')
            trailingActionBar.actions: [
                Action {
                    iconName: "info"
                    text: "info"
                    onTriggered: pageStack.push("qrc:/About.qml")
                }
            ]
        }

        Column {
            anchors.top: header.bottom
            spacing: units.gu(4)
            width: units.gu(18)
            anchors.centerIn: parent


            Button {
                text: archiveManager.name === "" ? i18n.tr("new Archive") : archiveManager.name
                width: parent.width
                visible: archiveManager.hasData
                color: theme.palette.normal.positive
                onClicked: {
                    pageStack.push("qrc:/ArchiveExplorer.qml", { archiveManager: archiveManager});
                }
            }

            Button {
                text: i18n.tr("Open archive")
                width: parent.width
                color: theme.palette.normal.positiveText
                onClicked: {
                   cleanup();
                   onClicked: pageStack.push(importPicker)
                }
            }

            Button {
                width: parent.width
                color: theme.palette.normal.positiveText

                onClicked: {
                    archiveManager.clear();
                    pageStack.push("qrc:/ArchiveWriter.qml", { archiveManager: archiveManager});
                }
                text: i18n.tr("Create archive")
            }
        }

    }

    PageStack {
        id: pageStack
        anchors.fill: parent
        Component.onCompleted: {
            pageStack.push(home)
        }
    }

    Connections {
        id: chConnection
        target: ContentHub

        onImportRequested: {
            console.log('onImportRequested Main')
            if (transfer.state === ContentTransfer.Charged) {

                var files = [];
                for (let i=0; i < transfer.items.length; i++) {
                    files.push(String(transfer.items[i].url).replace('file://', ''));
                }

                //var files = transfer.items.map((item) => String(item.url).replace('file://', ''));
                // we can only address one zip file at once
                if (archiveManager.isArchiveFile(files[0])) {
                    archiveManager.archive = files[0];
                    pageStack.push("qrc:/ArchiveExplorer.qml", { archiveManager: archiveManager});
                } else {
                    console.log('currentDir', archiveManager.currentDir)
                    // was on an archive read before, clear it
                    if (archiveManager.archive !== "") {
                        if (pageStack.depth > 1) {
                            pageStack.pop();
                        }

                        archiveManager.clear()
                    }
                    // first time
                    if (!archiveManager.hasData) {
                       pageStack.push("qrc:/ArchiveWriter.qml", { archiveManager: archiveManager})
                    }
                    files.forEach( file => archiveManager.appendFile(file, archiveManager.currentDir));


                }

            }
        }
    }

    Connections {
        target: Qt.application
        onAboutToQuit: {
            cleanup()
        }
    }

    Component.onCompleted: {
        console.log(pageStack.currentPage, archiveManager.rowCount())
        //console.log(archiveManager.isArchiveFile('/home/lduboeuf/.local/share/utzip.lduboeuf/utzip.tar.xz'));
        //archiveManager.appendFile("/home/lduboeuf/.local/share/utzip.lduboeuf/debug_content_hub", "");
       // pageStack.push("qrc:/ArchiveWriter.qml", { archiveManager: archiveManager});



        //archiveManager.archive = "/home/lduboeuf/.local/share/utzip.lduboeuf/utzip.tar.xz"
        //pageStack.push("qrc:/ArchiveExplorer.qml", { archiveManager: archiveManager});
        //pageStack.push("qrc:/ArchiveWriter.qml", { initialFiles: ["/home/lduboeuf/.local/share/utzip.lduboeuf/debug_content_hub"]})



    }



}
