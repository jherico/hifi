import QtQuick 2.6
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import QtMultimedia 5.6
import QtWebEngine 1.2

Window {
    visible: true
    width: 800
    height: 600

    Rectangle {
        id: rectangle1
        anchors.fill: parent

        WebEngineView {
            id: web
            objectName: "web"
            anchors.fill: parent
            url: "https://www.youtube.com/watch?v=Y6j8ZOJPoho"
            onFeaturePermissionRequested: {
                console.log("Feature permission requested");
                grantFeaturePermission(securityOrigin, feature, true);
            }
        }

        Component.onCompleted: {
            // Ensure the JS from the web-engine makes it to our logging
            web.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
                console.log("Web Entity JS message: " + sourceID + " " + lineNumber + " " +  message);
            });
        }
        
        ComboBox {
            id: text
            anchors.centerIn: parent
            model: AudioHandler.audioDevices
        }

        Audio {
            id: audio
            objectName: "audio"
            autoPlay: true
            source: "./TARDIS_Hum_10th.wav"
            loops: MediaPlayer.Infinite
            onPlaybackStateChanged: {
                console.log("playbackState " + audio.playbackState)
            }
        }

        Button {
            id: stop
            x: 46
            y: 168
            onClicked: {
                audio.pause()
            }
            text: "Stop"
            anchors.top: text.bottom
            anchors.topMargin: 0
            anchors.left: text.horizontalCenter
        }
        Button {
            id: play
            x: 245
            onClicked: {
                audio.play()
            }
            text: "Play"
            anchors.top: text.bottom
            anchors.topMargin: 0
            anchors.right: text.horizontalCenter
        }
        Button {
            id: change
            x: 245
            onClicked: {
                var script = '
                (function() { 
                    navigator.mediaDevices.getUserMedia({ audio: true, video: true }).then(function(mediaStream) {
                        navigator.mediaDevices.enumerateDevices().then(function(devices) {
                            var audioOutputs = [];
                            devices.forEach(function(device) {
                                if (device.kind == "audiooutput") {
                                    audioOutputs.push(device);
                                    console.log(device.kind + ": " + device.label + " id = " + device.deviceId);
                                }
                            });
                            var videos = document.getElementsByTagName("video");
                            videos[0].setSinkId(audioOutputs[3].deviceId);
                        }).catch(function(err) {
                            console.log(err.name + ": " + err.message);
                        });
                    }).catch(function(err) {
                        console.log(err.name + ": " + err.message);
                    });
                    
                })()';
                web.runJavaScript(script, function(result) { console.log(result); });
            }
            text: "change"
            anchors.top: play.bottom
            anchors.topMargin: 0
            anchors.right: text.horizontalCenter
        }
    }
}
