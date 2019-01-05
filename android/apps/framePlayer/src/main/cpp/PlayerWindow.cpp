//
//  Created by Bradley Austin Davis on 2018/10/21
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PlayerWindow.h"

#include <android/log.h>
#include <QtCore/QCoreApplication>

PlayerWindow::PlayerWindow() {
    QCoreApplication::processEvents();
    _renderThread.initialize();
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]{
        __android_log_write(ANDROID_LOG_WARN,"QQQ_LIFE", "application about to quit, stopping thread");
       _renderThread.terminate();
        deleteLater();
    });

}

PlayerWindow::~PlayerWindow() {
}
