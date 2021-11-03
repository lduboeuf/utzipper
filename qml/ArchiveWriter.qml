import QtQuick 2.7
import QtQuick.Layouts 1.3
import Ubuntu.Components 1.3
import Ubuntu.Content 1.3
import Ubuntu.Components.Popups 1.3
import Qt.labs.folderlistmodel 2.12

import ArchiveManager 1.0

Page {
    id: root
    anchors.fill: parent
    objectName: "ArchiveWriter"

    property ArchiveManager archiveManager: null
    property string archive: null
    property var navigation: []

    function save(archiveName, suffix) {

        const name = archiveName.replace(/[\s\?\[\]\/\\=<>:;,\'"&\$#*()|~`!{}%+]+/gi, '_');
        console.log('name:', name)
        const archivePath = archiveManager.save(name, suffix)
        if (archivePath !== "") {
            pageStack.push(exportPicker, { files: [archivePath]})
        } else {
            console.warn('error while exporting')
            //TODO errorMsg
        }
    }

    header: PageHeader {
        id: header
        subtitle: 'UT zipper'
        title: i18n.tr("new Archive")
        leadingActionBar.actions: [
            Action {
                iconName: "close"
                onTriggered: {
                    pageStack.pop()
                    archiveManager.currentDir = ""
                }
            }
        ]
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
            //numberOfSlots: 2

            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                leftMargin: units.gu(1)
            }
            actions: [
                Action {
                    iconName: "keyboard-caps-disabled"
                    text: archiveManager.currentDir
                    visible: archiveManager.currentDir !== ""
                    onTriggered: archiveManager.currentDir = root.navigation.pop()
                },
                Action {
                    iconName: "go-home"
                    text: i18n.tr("home")
                    onTriggered: {
                        archiveManager.currentDir = ""
                        root.navigation = []
                    }
                }
            ]

            delegate: AbstractButton {
                id: button1
                action: modelData
                width: label1.width + icon1.width + units.gu(3)
                height: parent.height
                Rectangle {
                    color: UbuntuColors.slate
                    opacity: 0.1
                    anchors.fill: parent
                    visible: button1.pressed
                }
                Icon {
                    id: icon1
                    anchors.verticalCenter: parent.verticalCenter
                    name: action.iconName
                    width: units.gu(2)
                }

                Label {
                    anchors.centerIn: parent
                    anchors.leftMargin: units.gu(2)
                    id: label1
                    text: action.text
                    font.weight: text === "Confirm" ? Font.Normal : Font.Light
                }
            }

            ActionBar {
                anchors.right: parent.right
                anchors.rightMargin: units.gu(1)
                actions: [
                    Action {
                        iconName: "import"
                        text: i18n.tr("add files")
                        onTriggered: pageStack.push(importPicker, { newArchive: true })
                    },
                    Action {
                        iconName: "tab-new"
                        text: i18n.tr("new folder")
                        onTriggered: PopupUtils.open(addFolderDialog)
                    }
                ]
            }
        }
    }

    FolderListModel {
        id: folderModel
        rootFolder: "file://" + archiveManager.newArchiveDir
        folder: "file://" +  archiveManager.newArchiveDir + "/" + archiveManager.currentDir
        onFolderChanged: {
            console.log('kikou folderModel', folder)
        }
        showDirsFirst: true
        showHidden: true
        Component.onCompleted: {
            console.log('folder', folder, 'rootFolder:', rootFolder)
        }
    }

    ListView {
        id: listView
        anchors {
            top: header.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        model: folderModel
        property int draggedIndex: -1

        currentIndex: -1
        delegate: ListItem {
            id: delegate
            height: layout.height + (divider.visible ? divider.height : 0)
            color:  index === listView.currentIndex ? theme.palette.selected.foreground : "transparent"
            //highlightColor: "blue"
            ListItemLayout {
                id: layout
                title.text: fileName

                Icon {
                    name: fileIsDir ? "document-open" : "stock_document"
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
                            const parentFolder = String(folderModel.folder).replace('file://', '')
                            if (fileIsDir) {
                                archiveManager.removeFolder(fileName, parentFolder)
                                archiveManager.currentDir = root.navigation.pop()
                            }else {
                                archiveManager.removeFile(fileName, parentFolder)
                            }
                        }
                    }
                ]
            }
            onClicked:  {
                if (fileIsDir) {
                    let tmpNav = root.navigation
                    tmpNav.push(archiveManager.currentDir)
                    root.navigation = tmpNav

                    if (archiveManager.currentDir !== "") {
                        archiveManager.currentDir = archiveManager.currentDir + "/" + fileName
                    } else {
                        archiveManager.currentDir = fileName
                    }

                }
            }
            onPressAndHold: {
                ListView.view.ViewItems.dragMode = !ListView.view.ViewItems.dragMode
            }
        }

        ViewItems.onDragUpdated: {
            if (event.status == ListItemDrag.Started) {
                console.log("from:", event.from)
                listView.draggedIndex = event.from
            } else if (event.status == ListItemDrag.Moving) {
                //console.log('event moving', event.from, event.to)
                const idx = event.to
                console.log("currentInde", listView.currentIndex, event.to)
                if (folderModel.get(event.to, "fileIsDir")) {
                    //listView.children[event.to].selected = true
                    listView.currentIndex = event.to
                    //listView.currentIndex = event.to
                }


            } else if (event.status == ListItemDrag.Dropped) {
                console.log('event from', draggedIndex, event.to)
                archiveManager.move(folderModel.get(draggedIndex, "fileURL"), folderModel.get(event.to, "fileURL"))
                listView.currentIndex = -1

                //model.move(event.from, event.to, 1);
            }
        }

        moveDisplaced: Transition {
            UbuntuNumberAnimation {
                property: "y"
            }
        }

    }

    Label {
        id: errorMsg
        anchors.centerIn: parent
        visible: archiveManager.error != ArchiveManager.NO_ERRORS
        text: i18n.tr("Oups, something went wrong");
    }

    AbstractButton {
        id: importBtn
        anchors.centerIn: parent
        width: importBtnLabel.width + units.gu(3)
        height: width
        visible: listView.count < 4
        Rectangle {
            color: UbuntuColors.slate
            opacity: 0.1
            anchors.fill: parent
            visible: importBtn.pressed
        }
        Column {
            spacing: units.gu(1)
            anchors.centerIn: parent

            Icon {
                anchors.horizontalCenter: parent.horizontalCenter
                name: "import"
                width: units.gu(4)
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                id: importBtnLabel
                text: i18n.tr("Import files")
                font.weight: text === "Confirm" ? Font.Normal : Font.Light
            }
        }
        onTriggered: pageStack.push(importPicker, { newArchive: true })
    }

    Component {
        id: saveDialog
        Dialog {
            id: dialogue
            title: i18n.tr("Export archive")

            Column {
                spacing: units.gu(2)

                TextField {
                    id: nametxt
                    placeholderText: i18n.tr("my archive name")
                    Layout.fillWidth: true
                    focus: true
                    Keys.onReturnPressed: saveBtn.clicked()
                    inputMethodHints: Qt.ImhUrlCharactersOnly
                }

                OptionSelector {
                    id: formatList
                    Layout.fillWidth: true
                    text: i18n.tr("format")
                    model: ["zip", "tar", "tar.gz", "tar.bz2", "tar.xz","7z"]
                }

                RowLayout {
                    width: parent.width
                    Button {
                        text: i18n.tr("cancel")
                        Layout.fillWidth: true
                        color: theme.palette.normal.base
                        onClicked: PopupUtils.close(dialogue)
                    }
                    Button {
                        id: saveBtn
                        text: i18n.tr("save")
                        Layout.fillWidth: true
                        color: theme.palette.normal.positive
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
            __closeOnDismissAreaPress: true
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
                    Keys.onReturnPressed: okBtn.clicked()
                    focus: true
                    Layout.fillWidth: true
                }

                RowLayout {
                    width: parent.width
                    Button {
                        text: i18n.tr("cancel")
                        Layout.fillWidth: true
                        color: theme.palette.normal.base
                        onClicked: PopupUtils.close(addFolderDialogue)
                    }
                    Button {
                        id: okBtn
                        text: i18n.tr("ok")
                        Layout.fillWidth: true
                        color: theme.palette.normal.positive
                        enabled: folderNametxt.inputMethodComposing || folderNametxt.displayText.length > 0
                        onClicked: {
                            const ok = archiveManager.appendFolder(folderNametxt.displayText, archiveManager.currentDir)
                            if (ok) {
                                PopupUtils.close(addFolderDialogue)
                            }
                        }
                        Keys.onReturnPressed: clicked()
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        if (root.archive) {
            archiveManager.extractTo(root.archive, archiveManager.newArchiveDir)
        }
    }

}
