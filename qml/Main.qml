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

import ArchiveReader 1.0

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
    }

    Page {
        id: picker
        visible: false
        header: PageHeader {
            id: pickerHeader
            title: i18n.tr("Choose from")
        }

        ContentPeerPicker {
            id: peerPicker
            //visible: parent.visible
            anchors.top: pickerHeader.bottom
            anchors.topMargin: units.gu(1)
            handler: ContentHandler.Source
            contentType: ContentType.Documents
            showTitle: false

            onPeerSelected: {
                peer.selectionType = ContentTransfer.Single;
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
            width: units.gu(16)
            anchors.centerIn: parent

            Button {
                text: i18n.tr("Open archive")
                width: parent.width
                color: theme.palette.normal.positiveText
                onClicked: {
                   cleanup();
                   onClicked: pageStack.push(picker)
                }
            }

            Button {
                width: parent.width
                color: theme.palette.normal.positiveText

                enabled: false
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
            if (transfer.state === ContentTransfer.Charged) {
                if (transfer.items.length > 0) {
                    activeTransfer = transfer
                    var filePath = String(transfer.items[0].url).replace('file://', '');
                    pageStack.push("qrc:/ArchiveExplorer.qml", { archive: filePath})
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
        //pageStack.push("qrc:/ArchiveExplorer.qml", { archive: "/home/lduboeuf/.local/share/utzip.lduboeuf/utzip.tar.xz"})
    }



}
