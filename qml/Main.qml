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
import Ubuntu.Components.Popups 1.3

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
        console.log('cleanup archive');
        archiveManager.clear();
    }

    function onImportedFiles(files) {

        // we can only address one zip file at once
        if (archiveManager.isArchiveFile(files[0])) {
            archiveManager.archive = files[0];
            pageStack.push("qrc:/ArchiveExplorer.qml", { archiveManager: archiveManager});
        } else {
            if (pageStack.currentPage.objectName !== "ArchiveWriter") {
                // we need to ask
                const popup = PopupUtils.open(newArchiveDialog);
                popup.confirmed.connect(function() {
                    // was on an archive read before, clear it
                    if (archiveManager.archive !== "") {
                        if (pageStack.depth > 1) {
                            pageStack.pop();
                        }
                    }
                    //archiveManager.clear()
                    files.forEach( file => archiveManager.appendFile(file, archiveManager.currentDir));
                    pageStack.push("qrc:/ArchiveWriter.qml", { archiveManager: archiveManager})
                });
            } else {
                // otherwise just add to the existing archive
                files.forEach( file => archiveManager.appendFile(file, archiveManager.currentDir));
            }
        }
    }

    ArchiveManager {
        id: archiveManager
    }

    Page {
        id: home
        anchors.fill: parent
        visible: false

        onVisibleChanged: {
            if (visible) {
                currentBtn.visible = archiveManager.hasData()
            }
        }

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
            id: menu
            anchors.top: header.bottom
            spacing: units.gu(4)
            width: units.gu(18)
            anchors.centerIn: parent


            Button {
                id: currentBtn
                text: i18n.tr("Current archive")
                width: parent.width
                visible: false
                color: theme.palette.normal.positive
                onClicked: {
                    if (archiveManager.name === "") {
                        pageStack.push("qrc:/ArchiveWriter.qml", { archiveManager: archiveManager});
                    } else {
                         pageStack.push("qrc:/ArchiveExplorer.qml", { archiveManager: archiveManager});
                    }
                }
            }

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

    Page {
        id: importPicker
        visible: false
        header: PageHeader {
            id: importPickerHeader
            title: i18n.tr("Choose from")
        }
        property bool newArchive: false

        ContentPeerPicker {
            //visible: parent.visible
            anchors.top: importPickerHeader.bottom
            anchors.topMargin: units.gu(1)
            handler: ContentHandler.Source
            contentType: importPicker.newArchive ? ContentType.All : ContentType.Documents
            showTitle: false

            onPeerSelected: {
                peer.selectionType = importPicker.newArchive ? ContentTransfer.Multiple : ContentTransfer.Single;
                root.activeTransfer = peer.request();
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
                                  console.log('added:', file)
                    exportPicker.selectedItems.push(resultComponent.createObject(root, {"url": "file://" + file}));
                })
                peer.selectionType = ContentTransfer.Single;
                root.activeTransfer = peer.request();
                root.activeTransfer.stateChanged.connect(function() {
                    if (root.activeTransfer.state === ContentTransfer.InProgress) {
                        root.activeTransfer.items = exportPicker.selectedItems;
                        root.activeTransfer.state = ContentTransfer.Charged;
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
                    text: i18n.tr("Sorry, not a supported archive file, would you like to start creating an archive ?")
                }

                RowLayout {
                    width: parent.width
                    Button {
                        text: i18n.tr("cancel")
                        Layout.fillWidth: true
                        onClicked: PopupUtils.close(newArchiveDialogue)
                    }
                    Button {
                        text: i18n.tr("ok")
                        Layout.fillWidth: true
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
        activeTransfer: root.activeTransfer
    }

    Connections {
        id: chConnection
        target: ContentHub

        onImportRequested: {
            if (transfer.state === ContentTransfer.Charged) {

                if (pageStack.currentPage.objectName !== "ArchiveWriter") {
                    cleanup();
                }

                var files = [];
                for (let i=0; i < transfer.items.length; i++) {
                    const item = transfer.items[i];
                    if (item.move(archiveManager.tempDir)){
                        files.push(String(item.url).replace('file://', ''));
                    }
                }
                console.log('output', files);
                transfer.finalize();
                onImportedFiles(files)

            }
        }
    }

    Connections {
        target: Qt.application
        onAboutToQuit: {
            console.log('aboutToQuit');
            cleanup()
        }
    }

    Component.onCompleted: {
        //console.log(archiveManager.isArchiveFile('/home/lduboeuf/.local/share/utzip.lduboeuf/utzip.tar.xz'));
        //archiveManager.appendFile("/home/lduboeuf/.local/share/utzip.lduboeuf/debug_content_hub", "");
        //pageStack.push("qrc:/ArchiveWriter.qml", { archiveManager: archiveManager});



        //archiveManager.archive = "/home/lduboeuf/.local/share/utzip.lduboeuf/utzip.tar.xz"
        //pageStack.push("qrc:/ArchiveExplorer.qml", { archiveManager: archiveManager});
        //pageStack.push("qrc:/ArchiveWriter.qml", { initialFiles: ["/home/lduboeuf/.local/share/utzip.lduboeuf/debug_content_hub"]})

    }
}
