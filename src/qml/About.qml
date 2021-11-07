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

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            width: units.gu(12)
            fillMode: Image.PreserveAspectFit
            source: "../../assets/logo.svg"
        }

        Label {
            fontSize: "large"
            anchors.horizontalCenter: parent.horizontalCenter
            text: i18n.tr("Archive Reader - Writer")
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: i18n.tr("Should support archive files like:")
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "zip, gzip, bzip, xz, tar, 7z"
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: i18n.tr("Source code:") + " <a href=\"https://github.com/lduboeuf/utzipper\">https://github.com/lduboeuf/utzipper</a>"
            onLinkActivated: Qt.openUrlExternally(link)
            wrapMode: Label.WordWrap
        }
    }
}
