import QtQuick 2.7
import Ubuntu.Components 1.3

Page {
    id: home
    anchors.fill: parent
    visible: false

    header: PageHeader {
        id: header
        title: i18n.tr('UT zipper')
    }


    Column {
        anchors.centerIn: parent
        spacing: units.gu(2)

        Label {
            //TODO size
            fontSize: "large"
            anchors.horizontalCenter: parent.horizontalCenter
            text: i18n.tr("Archive Reader/Writer")
        }

        Label {
            //anchors.horizontalCenter: parent.horizontalCenter
            text: i18n.tr("Should support archive files like zip, gzip, bzip, xz, tar, 7z")
        }

    }



}
