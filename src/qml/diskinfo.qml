import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1

Item {
    id: root
    
    property int reducedWidth: width - 25
    
    property string boxColor: "#475057"
    property string boxTextColor: "white"
    property int boxHeight: 30
    property int boxTextSize: 12
    
    property int tableKeyCellWidth: 200
    property int tableCellHeight: 27
    
    property int gridLayoutTextSize: 11
    property real gridLayoutLeftPadding: 9.0
    property color gridLayoutColor2: "#f5f5f5"
    property color gridLayoutColor1: "#fcfcfc"
    
    Text {
        visible: !isMediumPresent
        x: 8
        y: 4
        color: "black"
        leftPadding: gridLayoutLeftPadding
        text: noMediumLabel
        font.bold: true
        horizontalAlignment: Text.AlignLeft
        font.pointSize: boxTextSize
    }

    ScrollView {
        visible: isMediumPresent
        horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
        width: root.width
        height: root.height

        Column {
            width: reducedWidth

            Rectangle {
                width: reducedWidth
                height: boxHeight
                color: boxColor

                Text {
                    x: 8
                    y: 4
                    color: boxTextColor
                    text: mediumLabel
                    font.bold: true
                    horizontalAlignment: Text.AlignLeft
                    font.pointSize: boxTextSize
                }
            }

            TableView {                
                headerVisible: false
                selectionMode: SelectionMode.NoSelection
                horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
                verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff
                width: reducedWidth
                height: rowCount*tableCellHeight
                frameVisible: false
                TableViewColumn {
                    role: "key"
                    title: "Key"
                    width: tableKeyCellWidth
                }
                TableViewColumn {
                   role: "value"
                    title: "Value"
                    width: reducedWidth-tableKeyCellWidth
                }
                model: mediumTable
            }

            Rectangle {
                visible: isIsoFilesystem
                width: reducedWidth
                height: boxHeight
                color: boxColor

                Text {
                    x: 8
                    y: 4
                    color: boxTextColor
                    text: isoLabel
                    font.bold: true
                    horizontalAlignment: Text.AlignLeft
                    font.pointSize: boxTextSize
                }
            }

            TableView {
                visible: isIsoFilesystem
                headerVisible: false
                selectionMode: SelectionMode.NoSelection
                horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
                verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff
                //Layout.fillWidth: true
                width: reducedWidth
                height: rowCount*tableCellHeight
                frameVisible: false
                TableViewColumn {
                    role: "key"
                    title: "Key"
                    width: tableKeyCellWidth
                }
                TableViewColumn {
                   role: "value"
                   title: "Value"
                   width: reducedWidth-tableKeyCellWidth
                }
                model: filesystemInfoTable
            }

            Rectangle {
                visible: isTocNotEmpty
                width: reducedWidth
                height: boxHeight
                color: boxColor

                Text {
                    x: 8
                    y: 4
                    color: boxTextColor
                    text: tracksLabel
                    font.bold: true
                    horizontalAlignment: Text.AlignLeft
                    font.pointSize: boxTextSize
                }
            }
            
            GridLayout {
                id: gridLayout
                width: reducedWidth
                height: tableCellHeight*tracksGrid.getRowCount(gridLayout.columns)

                columnSpacing: 0
                rowSpacing: 0

                columns: 7
                rows: tracksGrid.getRowCount(columns)

                property real columnWidth: width/columns

                Repeater {
                    model: tracksGrid

                    delegate: Rectangle {
                        id: rect
                        Layout.columnSpan: colspan
                        width: gridLayout.columnWidth*colspan
                        height: tableCellHeight
                        color: tracksGrid.isRowEvenNumbered(gridLayout.columns, index) ? gridLayoutColor1 : gridLayoutColor2
                        Text {
                            anchors.verticalCenter: rect.verticalCenter
                            leftPadding: gridLayoutLeftPadding
                            width: gridLayout.columnWidth*colspan
                            text: itemText
                            font.pointSize: gridLayoutTextSize
                        }
                    }
                }
            }
        }
    }
}
