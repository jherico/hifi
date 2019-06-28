//
//  WebView.qml
//
//  Created by Bradley Austin Davis on 12 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtWebEngine 1.5

WebEngineView {
    id: root

    Component.onCompleted: {
        console.log("Connecting JS messaging to Hifi Logging")
        // Ensure the JS from the web-engine makes it to our logging
        root.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
            console.log("Web Window JS message: " + sourceID + " " + lineNumber + " " +  message);
        });
    }

    onLoadingChanged: {
        // Required to support clicking on "hifi://" links
        if (WebEngineView.LoadStartedStatus == loadRequest.status) {
            var url = loadRequest.url.toString();
            if (urlHandler.canHandleUrl(url)) {
                if (urlHandler.handleUrl(url)) {
                    root.stop();
                }
            }
        }
    }

    WebSpinner { }

    /* 
     * Async notification API placeholder START
     */
    Component {
        id: delayCallerComponent
        Timer { }
    }

    function delayCall( interval, callback ) {
        var delayCaller = delayCallerComponent.createObject( null, { "interval": interval } );
        delayCaller.triggered.connect( function () {
            callback();
            delayCaller.destroy();
        } );
        delayCaller.start();
    }
    /* 
     * Async notification API placeholder END
     */

    // FIXME replace use of Async notification API placeholder with actual user notification UI
    onFeaturePermissionRequested: {
        // Execute an asyncronous 
        delayCall(2000, function() {
            grantFeaturePermission(securityOrigin, feature, false);
        })
    }

}
