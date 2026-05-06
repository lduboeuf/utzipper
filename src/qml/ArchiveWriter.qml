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

    property string archive: ""
    property string sourcePassphrase: ""
    property var navigation: []
    property bool passwordDialogOpen: false

    function save(archiveName, suffix, passphrase) {

        const name = archiveName.replace(/[\s\?\[\]\/\\=<>:;,\'"&\$#*()|~`!{}%+]+/gi, '_');
        const archivePath = ArchiveManager.save(name, suffix, passphrase)
        if (archivePath !== "") {
            pageStack.push(exportPicker, { files: [archivePath]})
            return true
        } else {
            console.warn('error while exporting')
            //TODO errorMsg
            return false
        }
    }

    function extractArchive() {
        ArchiveManager.extractTo(root.archive, ArchiveManager.newArchiveDir, root.sourcePassphrase)
    }

    function openPasswordDialog() {
        if (passwordDialogOpen) {
            return
        }

        passwordDialogOpen = true
        const dialog = PopupUtils.open(passwordDialog, root, {
            invalidPassword: ArchiveManager.error === ArchiveManager.ERROR_INVALID_PASSPHRASE
        })
        dialog.accepted.connect(function(password) {
            root.sourcePassphrase = password
            extractArchive()
        })
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
                    color: LomiriColors.slate
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
        text: ArchiveManager.errorMessage !== "" ? ArchiveManager.errorMessage : i18n.tr("Oups, something went wrong")
    }


    AbstractButton {
        id: importBtn
        anchors.centerIn: parent
        width: importBtnLabel.width + units.gu(3)
        height: width
        visible: listView.count === 0
        Rectangle {
            color: LomiriColors.slate
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

            ColumnLayout {
                spacing: units.gu(2)
                anchors { left: parent.left; right: parent.right; }

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
                    model: ["zip", "tar", "tar.gz", "tar.bz2", "tar.xz", "7z", "rar"]
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    visible: formatList.model[formatList.selectedIndex] === "zip"
                    text: i18n.tr("Optional: set a passphrase to protect the exported ZIP archive.")
                }

                TextField {
                    id: passwordtxt
                    Layout.fillWidth: true
                    visible: formatList.model[formatList.selectedIndex] === "zip"
                    placeholderText: i18n.tr("passphrase (optional)")
                    echoMode: TextInput.Password
                }

                TextField {
                    id: passwordConfirmtxt
                    Layout.fillWidth: true
                    visible: formatList.model[formatList.selectedIndex] === "zip" && passwordtxt.displayText.length > 0
                    placeholderText: i18n.tr("confirm passphrase")
                    echoMode: TextInput.Password
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: theme.palette.normal.negative
                    visible: formatList.model[formatList.selectedIndex] === "zip"
                             && passwordtxt.displayText.length > 0
                             && passwordtxt.displayText !== passwordConfirmtxt.displayText
                    text: i18n.tr("The two passphrases must match.")
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: theme.palette.normal.negative
                    visible: !ArchiveManager.isWriteFormatSupported(formatList.model[formatList.selectedIndex])
                    text: i18n.tr("This format is read-only for now. Please choose zip, tar, tar.gz, tar.bz2, tar.xz or 7z.")
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
                        enabled: (nametxt.inputMethodComposing || nametxt.displayText.length > 0)
                                 && (formatList.model[formatList.selectedIndex] !== "zip"
                                     || passwordtxt.displayText.length === 0
                                     || passwordtxt.displayText === passwordConfirmtxt.displayText)
                                 && ArchiveManager.isWriteFormatSupported(formatList.model[formatList.selectedIndex])
                        onClicked: {
                            const selectedFormat = formatList.model[formatList.selectedIndex]
                            const exportPassphrase = selectedFormat === "zip" ? passwordtxt.displayText : ""
                            if (root.save(nametxt.displayText, selectedFormat, exportPassphrase)) {
                                PopupUtils.close(dialogue)
                            }
                        }
                    }
                }
            }

            onVisibleChanged: {
                if (!visible) {
                    root.passwordDialogOpen = false
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

    Component {
        id: passwordDialog

        Dialog {
            id: passwordDialogue
            title: i18n.tr("Protected ZIP archive")
            __closeOnDismissAreaPress: true

            property bool invalidPassword: false

            signal accepted(string password)

            Column {
                spacing: units.gu(2)

                Label {
                    width: parent.width
                    wrapMode: Label.WordWrap
                    text: passwordDialogue.invalidPassword
                          ? i18n.tr("The passphrase was not accepted. Please try again.")
                          : i18n.tr("Enter the passphrase to extract this ZIP archive before editing it.")
                }

                TextField {
                    id: sourcePassphraseField
                    Layout.fillWidth: true
                    placeholderText: i18n.tr("passphrase")
                    echoMode: TextInput.Password
                    focus: true
                    Keys.onReturnPressed: unlockArchiveButton.clicked()
                }

                RowLayout {
                    width: parent.width

                    Button {
                        text: i18n.tr("cancel")
                        Layout.fillWidth: true
                        color: theme.palette.normal.base
                        onClicked: {
                            root.passwordDialogOpen = false
                            PopupUtils.close(passwordDialogue)
                        }
                    }

                    Button {
                        id: unlockArchiveButton
                        text: i18n.tr("unlock")
                        Layout.fillWidth: true
                        color: theme.palette.normal.positive
                        enabled: sourcePassphraseField.displayText.length > 0
                        onClicked: {
                            root.passwordDialogOpen = false
                            passwordDialogue.accepted(sourcePassphraseField.displayText)
                            PopupUtils.close(passwordDialogue)
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: ArchiveManager

        onErrorChanged: {
            if (root.archive !== ""
                    && (ArchiveManager.error === ArchiveManager.ERROR_PASSPHRASE_REQUIRED
                        || ArchiveManager.error === ArchiveManager.ERROR_INVALID_PASSPHRASE)) {
                openPasswordDialog()
            }
        }
    }

    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: errorMsg.top
        visible: root.archive !== ""
                 && (ArchiveManager.error === ArchiveManager.ERROR_PASSPHRASE_REQUIRED
                     || ArchiveManager.error === ArchiveManager.ERROR_INVALID_PASSPHRASE)
        text: i18n.tr("Tap to enter passphrase")

        MouseArea {
            anchors.fill: parent
            onClicked: openPasswordDialog()
        }
    }

    Component.onCompleted: {
        if (root.archive !== "") {
            extractArchive()
        }
    }

}
