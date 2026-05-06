import QtQuick 2.7
import QtQuick.Layouts 1.3
import Lomiri.Components 1.3
import Lomiri.Content 1.3
import Lomiri.Components.Popups 1.3

import utzip.private 1.0

Page {
    id: root
    anchors.fill: parent
    objectName: "ArchiveReader"

    property var navigation: []
    property string archive: ""
    property bool passwordDialogOpen: false

    function selectAll() {
        const currentIndices = listView.ViewItems.selectedIndices
        let nextIndices = []
        for (let i=0; i < archiveReader.rowCount(); i++) {
            if (!archiveReader.get(i).isDir) {
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
        const files = selectedFiles.map(idx => archiveReader.get(idx).fullPath);
        console.log("files", files)
        const outFiles = ArchiveManager.extractFiles(archiveReader.archive, files, archiveReader.passphrase);
        console.log(outFiles)
        pageStack.push(exportPicker, { files: outFiles })
    }

    function openPasswordDialog() {
        if (passwordDialogOpen) {
            return
        }

        passwordDialogOpen = true
        const dialog = PopupUtils.open(passwordDialog, root, {
            invalidPassword: archiveReader.error === ArchiveReader.ERROR_INVALID_PASSPHRASE
        })
        dialog.accepted.connect(function(password) {
            archiveReader.passphrase = password
            archiveReader.retry()
        })
    }

    function canRetryWithPassphrase() {
        return archiveReader.error === ArchiveReader.ERROR_PASSPHRASE_REQUIRED
                || archiveReader.error === ArchiveReader.ERROR_INVALID_PASSPHRASE
    }

    header: PageHeader {
        id: header
        subtitle: 'UT zipper'
        title: archiveReader.name
        leadingActionBar.actions: [
            Action {
                iconName: "close"
                onTriggered: pageStack.pop()
            }
        ]
        trailingActionBar.actions: [
            Action {
                iconName: "share"
                enabled: listView.ViewItems.selectedIndices.length > 0 && archiveReader.error === ArchiveReader.NO_ERRORS
                onTriggered: share()
            },
            Action {
                iconName: "edit"
                enabled: archiveReader.error === ArchiveReader.NO_ERRORS
                onTriggered:  {
                    pageStack.push("qrc:/ArchiveWriter.qml", {
                        archive: archiveReader.archive,
                        sourcePassphrase: archiveReader.passphrase
                    });
                }
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
                    text: i18n.tr("up")
                    enabled: archiveReader.currentDir !== ""
                    onTriggered: archiveReader.currentDir = root.navigation.pop()
                },
                Action {
                    iconName: "go-home"
                    text: i18n.tr("home")
                    onTriggered: archiveReader.currentDir = ""
                }
            ]

            delegate: AbstractButton {
                id: button
                action: modelData
                width: label.width + icon.width + units.gu(3)
                height: parent.height
                Rectangle {
                    color: LomiriColors.slate
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
                        text: i18n.tr("select all")
                        enabled: archiveReader.hasFiles
                        onTriggered: selectAll()
                    }
                ]
            }
        }
    }

    ArchiveReader {
        id: archiveReader
        archive: root.archive
    }

    Label {
        id: errorMsg
        anchors.centerIn: parent
        visible: archiveReader.error != ArchiveReader.NO_ERRORS
        text: archiveReader.errorMessage !== "" ? archiveReader.errorMessage : i18n.tr("Oups, something went wrong")

        MouseArea {
            anchors.fill: parent
            enabled: canRetryWithPassphrase()
            onClicked: openPasswordDialog()
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
        model: archiveReader
        delegate: ListItem {
            height: layout.height + (divider.visible ? divider.height : 0)
            color:  selected ? theme.palette.selected.foreground : "transparent"
            ListItemLayout {
                id: layout
                title.text: name
                Icon {
                    name: isDir ? "document-open" : ArchiveManager.iconName(model.name)
                    SlotsLayout.position: SlotsLayout.Leading
                    width: units.gu(2)
                }
            }
            onClicked:  {
                if (!isDir) {
                    selected = !selected
                } else {
                    root.navigation.push(archiveReader.currentDir)
                    if (archiveReader.currentDir !== "") {
                        archiveReader.currentDir = archiveReader.currentDir + "/" + name
                    } else {
                        archiveReader.currentDir = name
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
                          : i18n.tr("Enter the passphrase to open this ZIP archive.")
                }

                TextField {
                    id: archivePassphraseField
                    width: parent.width
                    placeholderText: i18n.tr("passphrase")
                    echoMode: TextInput.Password
                    focus: true
                    Keys.onReturnPressed: unlockButton.clicked()
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
                        id: unlockButton
                        text: i18n.tr("unlock")
                        Layout.fillWidth: true
                        color: theme.palette.normal.positive
                        enabled: archivePassphraseField.displayText.length > 0
                        onClicked: {
                            root.passwordDialogOpen = false
                            passwordDialogue.accepted(archivePassphraseField.displayText)
                            PopupUtils.close(passwordDialogue)
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

    Connections {
        target: archiveReader

        onCurrentDirChanged: {
            listView.ViewItems.selectedIndices = []
        }

        onErrorChanged: {
            if (archiveReader.error === ArchiveReader.ERROR_PASSPHRASE_REQUIRED
                    || archiveReader.error === ArchiveReader.ERROR_INVALID_PASSPHRASE) {
                openPasswordDialog()
            }
        }
    }

    Component.onCompleted: {
        if (archiveReader.error === ArchiveReader.ERROR_PASSPHRASE_REQUIRED
                || archiveReader.error === ArchiveReader.ERROR_INVALID_PASSPHRASE) {
            openPasswordDialog()
        }
    }
}
