import QtQuick 2.7
import Ubuntu.Components 1.3
import Ubuntu.Content 1.3
import Ubuntu.Components.Popups 1.3

import ArchiveManager 1.0

Page {
    id: root
    anchors.fill: parent
    objectName: "ArchiveReader"

    property ArchiveManager archiveManager: null
    property var navigation: []

    function selectAll() {
        const currentIndices = listView.ViewItems.selectedIndices
        let nextIndices = []
        for (let i=0; i < archiveManager.rowCount(); i++) {
            if (!archiveManager.get(i).isDir) {
                nextIndices.push(i)
            }
        }
        if (currentIndices.length === nextIndices.length) {
            listView.ViewItems.selectedIndices = []
        } else {
            listView.ViewItems.selectedIndices = nextIndices
        }
    }

    function share() {
        const selectedFiles = listView.ViewItems.selectedIndices
        const files = selectedFiles.map(idx => archiveManager.get(idx).fullPath);
        const outFiles = archiveManager.extractFiles(files);
        pageStack.push(exportPicker, { files: outFiles })
    }

    header: PageHeader {
        id: header
        subtitle: 'UT zipper'
        //title: i18n.tr('UT zipper')
        title: archiveManager ? archiveManager.archive.replace(/^.*[\\\/]/, '') : ""
        leadingActionBar.actions: [
            Action {
                iconName: "close"
                onTriggered: pageStack.pop()
            }
        ]
        trailingActionBar.actions: [
            Action {
                iconName: "share"
                enabled: listView.ViewItems.selectedIndices.length > 0
                onTriggered: share()
            }
        ]
        extension:
            ActionBar {

            id: actionBar
            numberOfSlots: 2

            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                leftMargin: units.gu(1)
            }
            actions: [
                Action {
                    iconName: "keyboard-caps-disabled"
                    text: "up"
                    enabled: archiveManager.currentDir !== ""
                    onTriggered: archiveManager.currentDir = root.navigation.pop()
                },
                Action {
                    iconName: "go-home"
                    text: "home"
                    onTriggered: archiveManager.currentDir = ""
                }
            ]

            delegate: AbstractButton {
                id: button
                action: modelData
                width: label.width + icon.width + units.gu(3)
                height: parent.height
                Rectangle {
                    color: UbuntuColors.slate
                    opacity: 0.1
                    anchors.fill: parent
                    visible: button.pressed
                }
                Icon {
                    id: icon
                    anchors.verticalCenter: parent.verticalCenter
                    name: action.iconName
                    width: units.gu(2)
                }

                Label {
                    anchors.centerIn: parent
                    anchors.leftMargin: units.gu(2)
                    id: label
                    text: action.text
                    font.weight: text === "Confirm" ? Font.Normal : Font.Light
                }
            }

            ActionBar {
                anchors.right: parent.right
                anchors.rightMargin: units.gu(1)
                actions: [
                    Action {
                        iconName: "select"
                        text: "select all"
                        enabled: archiveManager.hasFiles
                        onTriggered: selectAll()
                    }
                ]
            }
        }
    }

    Label {
        id: errorMsg
        anchors.centerIn: parent
        visible: archiveManager.error != ArchiveManager.NO_ERRORS
        text: i18n.tr("Oups, something went wrong");
    }

    ListView {
        id: listView
        anchors {
            top: header.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        model: archiveManager
        delegate: ListItem {
            height: layout.height + (divider.visible ? divider.height : 0)
            color:  selected ? theme.palette.selected.foreground : "transparent"
            ListItemLayout {
                id: layout
                title.text: name
                Icon {
                    name: isDir ? "document-open" : "stock_document"
                    SlotsLayout.position: SlotsLayout.Leading
                    width: units.gu(2)
                }
            }
            onClicked:  {
                if (!isDir) {
                    selected = !selected
                } else {
                    root.navigation.push(archiveManager.currentDir)
                    if (archiveManager.currentDir !== "") {
                        archiveManager.currentDir = archiveManager.currentDir + "/" + name
                    } else {
                        archiveManager.currentDir = name
                    }

                }
            }
        }
    }

    Connections {
        target: archiveManager

        onCurrentDirChanged: {
            listView.ViewItems.selectedIndices = []
        }
    }
}
