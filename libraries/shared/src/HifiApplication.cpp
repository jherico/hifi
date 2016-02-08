//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HifiApplication.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QThread>
#include <QtCore/QStandardPaths>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#include "FileLogger.h"
#include "SettingHandle.h"
#include "LogHandler.h"
#include "PathUtils.h"
#include "PerfStat.h"
#include "NumericalConstants.h"

#ifndef BUILD_VERSION
#define BUILD_VERSION "unknown"
#endif

using namespace std;

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QString logMessage = LogHandler::getInstance().printMessage((LogMsgType) type, context, message);

    if (!logMessage.isEmpty()) {
#ifdef Q_OS_WIN
        OutputDebugStringA(logMessage.toLocal8Bit().constData());
        OutputDebugStringA("\n");
#endif
        qApp->getLogger()->addMessage(qPrintable(logMessage + "\n"));
    }
}

HifiApplication::HifiApplication(int& argc, char** argv) : QApplication(argc, argv) {
    // Set build version
    QCoreApplication::setApplicationVersion(BUILD_VERSION);

    // Set dependencies
    DependencyManager::set<PathUtils>();

    thread()->setPriority(QThread::HighPriority);
    thread()->setObjectName("Main Thread");

    _logger = new FileLogger(this);  // After setting organization name in order to get correct directory

    qInstallMessageHandler(messageHandler);

    qDebug() << "[VERSION] Build sequence:" << qPrintable(applicationVersion());

    connect(this, &QCoreApplication::aboutToQuit, this, &HifiApplication::onAboutToQuit);

    connect(this, &HifiApplication::applicationStateChanged, this, &HifiApplication::activeChanged);
    connect(&_idleTimer, &QTimer::timeout, [this] {
        idle(usecTimestampNow(), _lastTimeUpdated.nsecsElapsed() / NSECS_PER_USEC);
    });
    connect(this, &HifiApplication::beforeAboutToQuit, [=] {
        disconnect(&_idleTimer);
    });
    // Setting the interval to zero forces this to get called whenever there are no messages
    // in the queue, which can be pretty damn frequent.  Hence the idle function has a bunch 
    // of logic to abort early if it's being called too often.
    _idleTimer.start(0);
    _lastTimeUpdated.start();
}

void HifiApplication::onAboutToQuit() {
    _aboutToQuit = true;
    emit beforeAboutToQuit();
    cleanupBeforeQuit();
}

void HifiApplication::cleanupBeforeQuit() { 
}

HifiApplication::~HifiApplication() {
    qInstallMessageHandler(NULL); // NOTE: Do this as late as possible so we continue to get our log messages
}

void HifiApplication::idle(uint64_t nowUs, uint64_t elapsedUs) {
    if (_aboutToQuit) {
        return; // bail early, nothing to do here.
    }

    _lastTimeUpdated.start();

    {
        static uint64_t lastIdleStart { nowUs };
        uint64_t idleStartToStartDuration = nowUs - lastIdleStart;
        if (idleStartToStartDuration != 0) {
            _simsPerSecond.updateAverage((float)USECS_PER_SECOND / (float)idleStartToStartDuration);
        }
        lastIdleStart = nowUs;
    }

    {
        PerformanceTimer perfTimer("update");
        static const float BIGGEST_DELTA_TIME_SECS = 0.25f;
        update(glm::clamp((float)elapsedUs / (float)USECS_PER_SECOND, 0.0f, BIGGEST_DELTA_TIME_SECS));
    }
}

void HifiApplication::update(float deltaTime) {
}

void HifiApplication::activeChanged(Qt::ApplicationState state) {
    switch (state) {
        case Qt::ApplicationActive:
            _isForeground = true;
            break;

        case Qt::ApplicationSuspended:
        case Qt::ApplicationHidden:
        case Qt::ApplicationInactive:
        default:
            _isForeground = false;
            break;
    }
}

void HifiApplication::crashApplication() {
    qDebug() << "Intentionally crashed Interface";
    QObject* object = nullptr;
    bool value = object->isWindowType();
    Q_UNUSED(value);
}

float HifiApplication::getAverageSimsPerSecond() {
    uint64_t now = usecTimestampNow();

    if (now - _lastSimsPerSecondUpdate > USECS_PER_SECOND) {
        _simsPerSecondReport = _simsPerSecond.getAverage();
        _lastSimsPerSecondUpdate = now;
    }
    return _simsPerSecondReport;
}

