import QtQuick 2.7
import Ubuntu.Components 1.3
import Ubuntu.Content 1.3
import Ubuntu.Components.Popups 1.3

import ArchiveReader 1.0

Page {
    id: root
    anchors.fill: parent

    property alias archive: archiveModel.archive
    property var navigation: []
    property list<ContentItem> selectedItems
    property var activeTransfer

    function cleanup() {
        if (activeTransfer) {
            console.log('cleanup transfer');
            activeTransfer.finalize();
        }
    }

    function selectAll() {
        const currentIndices = listView.ViewItems.selectedIndices
        let nextIndices = []
        for (let i=0; i < archiveModel.rowCount(); i++) {
            if (!archiveModel.get(i).isDir) {
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
        selectedFiles.forEach(idx => {
                                  const path = archiveModel.get(idx).fullPath;
                                  const out = archiveModel.extractFile(path);
                                  selectedItems.push(resultComponent.createObject(root, {"url": "file://" + out}));
                              });
        pageStack.push(picker)
    }

    header: PageHeader {
        id: header
        title: i18n.tr('UT zipper')
        subtitle: archiveModel ? archive.replace(/^.*[\\\/]/, '') : ""
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
                    enabled: archiveModel.currentDir !== ""
                    onTriggered: archiveModel.currentDir = root.navigation.pop()
                },
                Action {
                    iconName: "go-home"
                    text: "home"
                    onTriggered: archiveModel.currentDir = ""
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
                        enabled: archiveModel.hasFiles
                        onTriggered: selectAll()
                    }
                ]
            }
        }
    }

    Label {
        id: errorMsg
        anchors.centerIn: parent
        visible: archiveModel.error != ArchiveReader.NO_ERRORS
        text: i18n.tr("Sorry, unsupported file format");
    }

    ArchiveReader {
        id: archiveModel
        property bool hasFiles: false
        currentDir: ""
        onCurrentDirChanged: {
            listView.ViewItems.selectedIndices = []
        }
        onRowCountChanged: {
            hasFiles = false;
            for (let i=0; i < archiveModel.rowCount(); i++) {
                console.log(archiveModel.get(i).name, archiveModel.get(i).isDir)
                if (!archiveModel.get(i).isDir) {
                    hasFiles = true;
                    return;
                }
            }
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
        model: archiveModel
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
                    root.navigation.push(archiveModel.currentDir)
                    if (archiveModel.currentDir !== "") {
                        archiveModel.currentDir = archiveModel.currentDir + "/" + name
                    } else {
                        archiveModel.currentDir = name
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

        ContentPeerPicker {
            id: peerPicker
            anchors.top: pickerHeader.bottom
            anchors.topMargin: units.gu(1)
            handler: ContentHandler.Destination
            contentType: ContentType.Documents
            showTitle: false

            onPeerSelected: {
                peer.selectionType = ContentTransfer.Multiple;
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

    Connections {
        target: Qt.application
        onAboutToQuit: {
            cleanup()
        }
    }
}
