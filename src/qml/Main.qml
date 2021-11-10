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

import QtQuick 2.7
import Ubuntu.Components 1.3
import QtQuick.Layouts 1.3
import Ubuntu.Content 1.3
import Qt.labs.settings 1.0
import Ubuntu.Components.Popups 1.3

import utzip.private 1.0

MainView {
    id: mainView
    objectName: 'mainView'
    applicationName: 'utzip.lduboeuf'
    automaticOrientation: true

    width: units.gu(45)
    height: units.gu(75)

    property var activeTransfer
    property string currentDir

    function cleanup() {
        console.log('cleanup');
        ArchiveManager.clear();
    }


    function onImport(transfer) {
        if (pageStack.currentPage.objectName !== "ArchiveWriter") {
             cleanup();
            if (pageStack.depth > 1) {
                pageStack.pop();
            }

            if (!ArchiveManager.isArchiveFile(transfer.items[0].url)){
                const popup = PopupUtils.open(newArchiveDialog);
                popup.confirmed.connect(function() {
                    moveFiles(transfer, ArchiveManager.currentDir)
                    pageStack.push("qrc:/ArchiveWriter.qml")
                });

            } else {
                const files = moveFiles(transfer, ArchiveManager.tempDir)
                pageStack.push("qrc:/ArchiveExplorer.qml", { archive: files[0]});
            }

        } else {
            // we are already in a new Archive mode, just add new files
            moveFiles(transfer, ArchiveManager.currentDir)
        }
    }

    function moveFiles(transfer, destinationDir) {
        var files = [];
        for (let i=0; i < transfer.items.length; i++) {
            const item = transfer.items[i];
            console.log("move to:", destinationDir)
            // we use custom copy here since content-hub contentItem.move() will copy twice the file
            // files in .cache/HubIncoming will be deleted on transfer.finalize()
            if (ArchiveManager.copy(item.url, destinationDir)){
                const fileName = item.url.toString().split('/').pop();
                files.push(destinationDir + "/" + fileName);
            }
        }

        transfer.finalize();
        return files;
    }

    Page {
        id: home
        anchors.fill: parent
        visible: false

        header: PageHeader {
            id: header
            title: 'UT zipper'
            trailingActionBar.actions: [
                Action {
                    iconName: "info"
                    text: i18n.tr("info")
                    onTriggered: pageStack.push("qrc:/About.qml")
                }
            ]
        }

        Column {
            id: menu
            anchors.top: header.bottom
            spacing: units.gu(4)
            width: units.gu(18)
            anchors.centerIn: parent

            Button {
                text: i18n.tr("Open archive")
                width: parent.width
                color: theme.palette.normal.positiveText
                onClicked: {
                   onClicked: pageStack.push(importPicker, { newArchive: false })
                }
            }

            Button {
                width: parent.width
                color: theme.palette.normal.positiveText

                onClicked: {
                    cleanup()
                    pageStack.push("qrc:/ArchiveWriter.qml");
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

    Page {
        id: importPicker
        visible: false
        header: PageHeader {
            id: importPickerHeader
            title: i18n.tr("Choose from")
        }
        property bool newArchive: false

        ContentPeerPicker {
            anchors.top: importPickerHeader.bottom
            anchors.topMargin: units.gu(1)
            handler: ContentHandler.Source
            contentType: importPicker.newArchive ? ContentType.All : ContentType.Documents
            showTitle: false

            onPeerSelected: {
                peer.selectionType = importPicker.newArchive ? ContentTransfer.Multiple : ContentTransfer.Single;
                mainView.activeTransfer = peer.request();
                pageStack.pop()
            }

            onCancelPressed: pageStack.pop();
        }
    }

    Page {
        id: exportPicker
        visible: false
        header: PageHeader {
            id: pickerHeader
            title: i18n.tr("Export to")
        }

        property var files: []
        property list<ContentItem> selectedItems

        ContentPeerPicker {
            id: peerPicker
            anchors.top: pickerHeader.bottom
            anchors.topMargin: units.gu(1)
            handler: ContentHandler.Destination
            contentType: ContentType.Documents
            showTitle: false

            onPeerSelected: {
                exportPicker.selectedItems = []
                exportPicker.files.forEach( file => {
                    console.log('export:', file)
                    exportPicker.selectedItems.push(resultComponent.createObject(mainView, {"url": file}));
                })
                peer.selectionType = ContentTransfer.Single;
                mainView.activeTransfer = peer.request();
                mainView.activeTransfer.stateChanged.connect(function() {
                    if (mainView.activeTransfer.state === ContentTransfer.InProgress) {
                        mainView.activeTransfer.items = exportPicker.selectedItems;
                        mainView.activeTransfer.state = ContentTransfer.Charged;
                        pageStack.pop()
                    }
                })
            }

            onCancelPressed: pageStack.pop();
        }

        Component {
            id: resultComponent
            ContentItem {}
        }
    }

    Component {
        id: newArchiveDialog
        Dialog {
            id: newArchiveDialogue
            title: i18n.tr("Unsupported archive format")

            property alias content : label.text

            signal confirmed()

            Column {
                spacing: units.gu(2)
                Label {
                    id: label
                    width: parent.width
                    wrapMode: Label.WordWrap
                    text: i18n.tr("Would you like to create one with that file(s) ?")
                }

                RowLayout {
                    width: parent.width
                    Button {
                        text: i18n.tr("no")
                        Layout.fillWidth: true
                        color: theme.palette.normal.base
                        onClicked: PopupUtils.close(newArchiveDialogue)
                    }
                    Button {
                        text: i18n.tr("yes")
                        Layout.fillWidth: true
                        color: theme.palette.normal.positive
                        focus: true
                        onClicked: {
                            confirmed()
                            PopupUtils.close(newArchiveDialogue)
                        }
                    }
                }
            }
        }
    }

    ContentTransferHint {
        anchors.fill: parent
        activeTransfer: mainView.activeTransfer
    }

    Connections {
        id: chConnection
        target: ContentHub

        onImportRequested: {
            if (transfer.state === ContentTransfer.Charged) {
               onImport(transfer)
            } else if (transfer.state === ContentTransfer.Finalized) {
                console.log('ContentTransfer.Finalize');

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
       //pageStack.push("qrc:/ArchiveExplorer.qml", { archive: 'file:///home/lduboeuf/.local/share/utzip.lduboeuf/utzip.tar.xz'});
       //pageStack.push("qrc:/ArchiveWriter.qml", { archive: 'file:///home/lduboeuf/.local/share/utzip.lduboeuf/utzip.tar.xz'});
    }
}
