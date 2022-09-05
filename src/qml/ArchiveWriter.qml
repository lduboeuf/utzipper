import QtQuick 2.7
import QtQuick.Layouts 1.3
import Lomiri.Components 1.3
import Lomiri.Content 1.3
import Lomiri.Components.Popups 1.3
import Qt.labs.folderlistmodel 2.12

import utzip.private 1.0

Page {
    id: root
    anchors.fill: parent
    objectName: "ArchiveWriter"

    property string archive: null
    property var navigation: []

    function save(archiveName, suffix) {

        const name = archiveName.replace(/[\s\?\[\]\/\\=<>:;,\'"&\$#*()|~`!{}%+]+/gi, '_');
        const archivePath = ArchiveManager.save(name, suffix)
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
                    ArchiveManager.currentDir = ArchiveManager.newArchiveDir
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
            enabled: !listView.ViewItems.dragMode
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                leftMargin: units.gu(1)
            }
            actions: [
                Action {
                    iconName: "keyboard-caps-disabled"
                    text: ArchiveManager.currentName
                    enabled: ArchiveManager.currentDir !== ArchiveManager.newArchiveDir
                    onTriggered: ArchiveManager.currentDir = root.navigation.pop()
                },
                Action {
                    iconName: "go-home"
                    text: i18n.tr("home")
                    onTriggered: {
                        ArchiveManager.currentDir = ArchiveManager.newArchiveDir
                        root.navigation = []
                    }
                }
            ]

            delegate: AbstractButton {
                id: button1
                action: modelData
                //anchors.right: rightActionBar.left
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
                    id: label1
                    anchors.centerIn: parent
                    anchors.leftMargin: units.gu(2)
                    elide: Label.ElideLeft
                    width: Math.min(units.gu(22), implicitWidth)
                    text: action.text
                    font.weight: Font.Light
                }
            }

            ActionBar {
                id: rightActionBar
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
                        //tab-new
                        text: i18n.tr("new folder")
                        onTriggered: PopupUtils.open(addFolderDialog)
                    }
                ]
            }
        }
    }

    FolderListModel {
        id: folderModel
        rootFolder: ArchiveManager.newArchiveDir
        folder: ArchiveManager.currentDir
        onFolderChanged: console.log('folder:', folder);
        showDirsFirst: true
        showHidden: true
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
            ListItemLayout {
                id: layout
                title.text: fileName

                Icon {
                    name: fileIsDir ? "document-open" : ArchiveManager.iconName(fileName)
                    SlotsLayout.position: SlotsLayout.Leading
                    width: units.gu(2)
                }

            }
            leadingActions: ListItemActions {
                actions: [
                    Action {
                        iconName: "delete"
                        text: i18n.tr("delete")
                        onTriggered: {
                            console.log('kikou remove:', fileURL)
                            if (fileIsDir) {
                                ArchiveManager.removeFolder(fileURL)
                            }else {
                                ArchiveManager.removeFile(fileURL)
                            }
                        }
                    }
                ]
            }
            onClicked:  {
                if (fileIsDir) {
                    let tmpNav = root.navigation
                    tmpNav.push(ArchiveManager.currentDir)
                    root.navigation = tmpNav

                    ArchiveManager.currentDir = Qt.resolvedUrl(ArchiveManager.currentDir.toString() + "/" + fileName)
                    ListView.view.ViewItems.dragMode = false

                }
            }
            onPressAndHold: {
                ListView.view.ViewItems.dragMode = !ListView.view.ViewItems.dragMode
            }
        }

        ViewItems.onDragUpdated: {
            if (event.status === ListItemDrag.Started) {
                listView.draggedIndex = event.from
            } else if (event.status === ListItemDrag.Moving) {
                const idx = event.to
                if (folderModel.get(event.to, "fileIsDir")) {
                    listView.currentIndex = event.to
                } else {
                    listView.currentIndex = -1
                }

            } else if (event.status === ListItemDrag.Dropped) {
                ArchiveManager.move(folderModel.get(draggedIndex, "fileURL"), folderModel.get(event.to, "fileURL"))
                listView.currentIndex = -1
            }
        }

        removeDisplaced: Transition {
                NumberAnimation { property: "y"; duration: 1000 }
        }
    }

    Label {
        id: errorMsg
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: importBtn.top
        visible: ArchiveManager.error != ArchiveManager.NO_ERRORS
        text: i18n.tr("Oups, something went wrong");
    }


    AbstractButton {
        id: importBtn
        anchors.centerIn: parent
        width: importBtnLabel.width + units.gu(3)
        height: width
        visible: listView.count === 0
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
                font.weight: Font.Light
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
                            const ok = ArchiveManager.appendFolder(folderNametxt.displayText, ArchiveManager.currentDir)
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
            ArchiveManager.extractTo(root.archive, ArchiveManager.newArchiveDir)
        }
    }

}
