//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_HifiApplication_h
#define hifi_HifiApplication_h

#include <functional>
#include <stdint.h>

#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QStringList>
#include <QtCore/QElapsedTimer>

#include <QtGui/qevent.h>
#include <QtGui/QImage>
#include <QtWidgets/QApplication>

#include "SimpleMovingAverage.h"

class FileLogger;

class HifiApplication : public QApplication  {
    Q_OBJECT

public:
    HifiApplication(int& argc, char** argv);
    virtual ~HifiApplication();

    FileLogger* getLogger() { return _logger; }

    bool isAboutToQuit() const { return _aboutToQuit; }

    virtual bool canAcceptURL(const QString& url) const { return false; }
    virtual bool acceptURL(const QString& url, bool defaultUpload = false) { return false; }
    bool isForeground() const { return _isForeground; }

signals:
    void beforeAboutToQuit();

public slots:
    void crashApplication();
    float getAverageSimsPerSecond();

protected slots:
    virtual void idle(uint64_t nowUs, uint64_t elapsedUs);
    virtual void onAboutToQuit();
    virtual void activeChanged(Qt::ApplicationState state);
protected:
    virtual void cleanupBeforeQuit();
    virtual void update(float deltaTime);

private:
    QElapsedTimer _lastTimeUpdated;
    bool _aboutToQuit { false };
    bool _isForeground { true };
    FileLogger* _logger;
    QTimer _idleTimer;
    SimpleMovingAverage _simsPerSecond { 10 };
    int _simsPerSecondReport { 0 };
    quint64 _lastSimsPerSecondUpdate { 0 };
};

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<HifiApplication*>(QCoreApplication::instance()))

#endif // hifi_Application_h
