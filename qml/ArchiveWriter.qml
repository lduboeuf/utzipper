import QtQuick 2.7
import QtQuick.Layouts 1.3
import Ubuntu.Components 1.3
import Ubuntu.Content 1.3
import Ubuntu.Components.Popups 1.3

import ArchiveManager 1.0

Page {
    id: root
    anchors.fill: parent

    property ArchiveManager archiveManager: null
    property var navigation: []
    property list<ContentItem> selectedItems
    property var activeTransfer

    function cleanup() {
        if (activeTransfer) {
            console.log('cleanup transfer');
            activeTransfer.finalize();
        }
    }

    function save(archiveName, suffix) {

        const name = archiveName.replace(/[\s\?\[\]\/\\=<>:;,\'"&\$#*()|~`!{}%+]+/gi, '_');
        console.log('name:', name)
        const archivePath = archiveManager.save(name, suffix)
        selectedItems = []
        if (archivePath !== "") {
            selectedItems.push(resultComponent.createObject(root, {"url": "file://" + archivePath}));
            pageStack.push(picker)
        } else {
            //TODO errorMsg
        }
    }

    header: PageHeader {
        id: header
        title: i18n.tr('UT zipper')
        subtitle: i18n.tr("new Archive")
        trailingActionBar.actions: [
            Action {
                iconName: "share"
                enabled: listView.count > 0
                onTriggered: PopupUtils.open(saveDialog)
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
                        iconName: "note-new"
                        text: i18n.tr("import file")
                        onTriggered: pageStack.push(importPicker)
                    }
//                    Action {
//                        iconName: "tab-new"
//                        text: i18n.tr("new folder")
//                        onTriggered: PopupUtils.open(addFolderDialog)
//                    }
                ]
            }
        }
    }

    Label {
        id: errorMsg
        anchors.centerIn: parent
        visible: archiveManager.error != ArchiveManager.NO_ERRORS
        text: i18n.tr("Sorry, unsupported file format");
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
            leadingActions: ListItemActions {
                actions: [
                    Action {
                        iconName: "delete"
                        text: "delete"
                        onTriggered: {
                            if (model.isDir) {
                                archiveManager.removeFolder(model.name, archiveManager.currentDir)
                            }else {
                                archiveManager.removeFile(model.name, archiveManager.currentDir)
                            }
                        }
                    }
                ]
            }
            onClicked:  {
                if (isDir) {
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

    Page {
        id: picker
        visible: false
        header: PageHeader {
            id: pickerHeader
            title: i18n.tr("Export to")
        }

        property var files: []

        ContentPeerPicker {
            id: peerPicker
            anchors.top: pickerHeader.bottom
            anchors.topMargin: units.gu(1)
            handler: ContentHandler.Destination
            contentType: ContentType.Documents
            showTitle: false

            onPeerSelected: {
                peer.selectionType = ContentTransfer.Single;
                root.activeTransfer = peer.request();
                root.activeTransfer.stateChanged.connect(function() {
                    if (root.activeTransfer.state === ContentTransfer.InProgress) {
                        console.log("Export: In progress, nb items:", selectedItems.length);
                        root.activeTransfer.items = selectedItems;
                        root.activeTransfer.state = ContentTransfer.Charged;
                        pageStack.pop()
                    }
                })
            }

            onCancelPressed: pageStack.pop();
        }
    }

    ContentTransferHint {
        id: transferHint
        anchors.fill: parent
        activeTransfer: root.activeTransfer
    }

    Component {
        id: resultComponent
        ContentItem {}
    }

    Component {
        id: saveDialog
        Dialog {
            id: dialogue
            title: i18n.tr("Export archive")

            Column {
                spacing: units.gu(2)
                Label {
                    id: label
                    text: i18n.tr("Archive name:")
                    elide: Text.ElideRight
                    font.weight: Font.Light
                }

                TextField {
                    id: nametxt
                    placeholderText: i18n.tr("new archive...")
                    Layout.fillWidth: true
                }

                OptionSelector {
                    id: formatList
                    Layout.fillWidth: true
                    text: i18n.tr("Archive format:")
                    model: ["zip", "tar", "7z"]
                }

                RowLayout {
                    width: parent.width
                    Button {
                        text: i18n.tr("cancel")
                        Layout.fillWidth: true
                        onClicked: PopupUtils.close(dialogue)
                    }
                    Button {
                        text: i18n.tr("save")
                        Layout.fillWidth: true
                        enabled: nametxt.inputMethodComposing || nametxt.displayText.length > 0
                        onClicked: {
                            root.save(nametxt.displayText, formatList.model[formatList.selectedIndex]);
                            PopupUtils.close(dialogue)
                        }
                    }
                }
            }
        }
    }


    Component {
        id: addFolderDialog
        Dialog {
            id: addFolderDialogue
            title: i18n.tr("Add folder")

            Column {
                spacing: units.gu(2)
                Label {
                    id: label
                    text: i18n.tr("Folder name")
                    elide: Text.ElideRight
                    font.weight: Font.Light
                }

                TextField {
                    id:folderNametxt
                    placeholderText: i18n.tr("new folder...")
                    Layout.fillWidth: true
                }

                Button {
                    text: i18n.tr("ok")
                    anchors.horizontalCenter: parent.horizontalCenter
                    //Layout.fillWidth: true
                    enabled: folderNametxt.inputMethodComposing || folderNametxt.displayText.length > 0
                    onClicked: {
                        archiveManager.appendFolder(folderNametxt.displayText, archiveManager.currentDir)
                        PopupUtils.close(addFolderDialogue)
                    }
                    Keys.onReturnPressed: clicked()
                }
            }
        }
    }

    Component.onCompleted: {
//        if (archiveManager) {
//            archiveManager.currentDir = ""
//        }
        console.log('pageStack nb:', pageStack.depth)
    }

    Connections {
        target: archiveManager

        onCurrentDirChanged: {
            listView.ViewItems.selectedIndices = []
        }
    }

    Connections {
        target: Qt.application
        onAboutToQuit: {
            cleanup()
        }
    }
}
