import Qt 4.7


Image {
    width: 600
    height: 300

    id: bg
    source: "rect27544.png"
    clip:true
    Image {
        id: cdrefexion2
        width:cd.width*1.2
        height:cd.width*1.2
        smooth: true
        x: cd.x
        y: bg.height/2
        anchors.horizontalCenter: cdcopy.horizontalCenter
        source: "image3870.png"
        rotation:-cdcopy.rotation
    }

    Image {
        id: cdrefexion1
        width:cd.width*1.2
        height:cd.width*1.2
        x: cd.x
        y: bg.height/2
        smooth: true
        anchors.horizontalCenter: cd.horizontalCenter
        source: "image27548.png"
        rotation:cd.rotation*-1
    }
    Image {
    id: cd2refexion1
    width:cd2.width*1.2
    height:cd2.width*1.2
    x: cd2.x
    y: bg.height/2
    smooth: true
    anchors.horizontalCenter: cd2.horizontalCenter
    source: "image27548.png"
    rotation:cd2.rotation*-1
}
    Image {
        id: cd2notereflexion
        source: "g5104.png"
        width:cd2note.width*1.2
        height:cd2note.height*1.2
        anchors.top: cd2note.bottom
        anchors.topMargin: -11
        anchors.horizontalCenter: cd2note.horizontalCenter
    }

    Text {
        id: textcopy
        x:text2.x
        y:text2.y+27
        color: "#ffffff"
        text: text2.text
        smooth: true
        transform: Scale { origin.x: 0; origin.y: 0; yScale: -1}
    }
    Text {
    id: textcopy2
    x:text1.x
    y:text1.y+27
    color: "#ffffff"
    text: text1.text
    smooth: true
    transform: Scale { origin.x: 0; origin.y: 0; yScale: -1}
}
    Image {
        //dirty trick to make a reflexion efect
        id: relexion
        y: bg.height/2-50
        width:bg.width
        source: "rect27550.png"
    }
    Image {
        width: cd.width
        height: cd.height
        id: cdcopy
        source: "media-optical-recordable.png"
        x:91+15
        y:bg.height/2-height+15
        smooth: true

    }
    Image {
        id: cd
        source: "media-optical.png"
        x:91+(cdarea.containsMouse?-20:0)
        y:bg.height/2-height+15
        rotation: 360
        z: 0
        smooth: true
        width:100+(cdarea.containsMouse?28:0)
        height:width
        MouseArea{
            id:cdarea
            anchors.fill: parent
            hoverEnabled:true
            onClicked: copyCdAction.trigger()
        }
        Behavior on x {
            NumberAnimation {
            duration: 1000
            easing.type: Easing.InOutCubic
            }
        }
        Behavior on width {
        NumberAnimation {
        duration: 1000
        easing.type: Easing.InOutCubic
        }
    }
    }
//    Image {
//        id: cdbase
//        source: "media-optical.png"
//        x:cd.x
//        y:cd.y

//        smooth: true
//        width:cd.width
//        height:cd.width
//        opacity:0.8

//    }
    Image {
        id: cd2
        source: "media-optical.png"
        y:bg.height/2-height+15
        rotation: 360
        z: 0
        smooth: true
        width:100+(cd2area.containsMouse?28:0)
        height:width
        anchors.left: cdcopy.right
        anchors.leftMargin: 30
        MouseArea{
            id:cd2area
            anchors.fill: parent
            hoverEnabled:true
            onClicked: audioCdAction.trigger()
        }
        Behavior on width {
        NumberAnimation {
        duration: 1000
        easing.type: Easing.InOutCubic
        }
    }
    }
    Image {
        id: cd2note
        height:60+(cd2area.containsMouse?18:0)
        width:height*65/101
        anchors.bottom:cd2.bottom
        x:cd2.x+50+(cd2.width/2-50)*2
        source: "layer1-5.png"
        Behavior on height {
            NumberAnimation {
            duration: 1000
            easing.type: Easing.InOutCubic
        }}
        anchors.bottomMargin: -4
        smooth: true

    }


    SequentialAnimation {
        running: cdarea.containsMouse
        NumberAnimation {
            target: cd
            property: "rotation"
            to: 0
            duration: 1200
            easing.type: Easing.InQuart
        }
        SequentialAnimation {
            id:blag

            loops:300
            NumberAnimation {
                target: cd
                property: "rotation"
                to: 360
                duration: 0
            }
            NumberAnimation {
                target: cd
                property: "rotation"
                to: 0
                duration: 300
            }

        }

    }
    SequentialAnimation {
        id:blagi
        running: !cdarea.containsMouse
        NumberAnimation {
            target: cd
            property: "rotation"
            to: -360
            duration: 1200
            easing.type: Easing.OutQuart
        }NumberAnimation {
        target: cd
        property: "rotation"
        to: 360
        duration: 0

    }
    }
    SequentialAnimation {
        running: cdarea.containsMouse
        PauseAnimation { duration: 1000 }
        NumberAnimation {
            target: cdcopy
            property: "rotation"
            to: 0
            duration: 1200
            easing.type: Easing.InQuart
        }
        SequentialAnimation {
            id:blag2

            loops:300

            NumberAnimation {
                target: cdcopy
                property: "rotation"
                to: 360
                duration: 0
            }
            NumberAnimation {
                target: cdcopy
                property: "rotation"
                to: 0
                duration: 300
            }

        }

    }
    SequentialAnimation {
        id:blagi2
        running: !cdarea.containsMouse

        NumberAnimation {
            target: cdcopy
            property: "rotation"
            to: 360
            duration: 0
        }
        NumberAnimation {
            target: cdcopy
            property: "rotation"
            to: 0
            duration: 300
        }NumberAnimation {
        target: cdcopy
        property: "rotation"
        to: 360
        duration: 0
    }
        NumberAnimation {
            target: cdcopy
            property: "rotation"
            to: 0
            duration: 300
        }
        NumberAnimation {
            target: cdcopy
            property: "rotation"
            to: -360
            duration: 1200
            easing.type: Easing.OutQuart
        }NumberAnimation {
        target: cdcopy
        property: "rotation"
        to: 360
        duration: 0

    }
    }
    Text {
        id: text1
        x: 111
        color: "#ffffff"
        text: "Copy cd"
        smooth: true
        anchors.top: cd.bottom
        anchors.topMargin: 0
        anchors.horizontalCenterOffset: 9
        anchors.horizontalCenter: cd.horizontalCenter
    }
    Text {
        id: text2
        x: 111
        color: "#ffffff"
        text: "Audio Cd"
        smooth: true
        anchors.top: cd2.bottom
        anchors.topMargin: 0
        anchors.horizontalCenterOffset: 1
        anchors.horizontalCenter: cd2.horizontalCenter
    }


    Image {
        id: k3b
        source: "text27519.png"
        x:367
        anchors.top: parent.top
        anchors.topMargin: 19
        anchors.right: parent.right
        anchors.rightMargin: 16

        smooth: true
    }
    Image {
        id: shade
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        source: "rect27552.png"

    }
    Image {
        id: deco
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        source: "image27566.png"
    }
    Image {
        id: deco2

        source: "g11556.png"
    }
}


