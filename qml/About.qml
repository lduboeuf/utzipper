import QtQuick 2.7
import Ubuntu.Components 1.3

Page {
    id: home
    anchors.fill: parent
    visible: false

    header: PageHeader {
        id: header
        title: i18n.tr('About')
        subtitle: "UT Zipper"
    }


    Column {
        anchors.centerIn: parent
        spacing: units.gu(3)

        Label {
            fontSize: "large"
            anchors.horizontalCenter: parent.horizontalCenter
            text: i18n.tr("Archive Reader - Writer")
        }

        Label {
            text: i18n.tr("Should support archive files like zip, gzip, bzip, xz, tar, 7z")
        }

        Label {
            text: i18n.tr("Source code:") + "<a href=\"https://github.com/lduboeuf/utzipper\">https://github.com/lduboeuf/utzipper</a>"
            onLinkActivated: Qt.openUrlExternally(link)
            wrapMode: Label.WordWrap
        }
    }
}
