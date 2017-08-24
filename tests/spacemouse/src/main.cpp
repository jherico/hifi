//
//  Created by Bradley Austin Davis on 2017/06/06
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <mutex>

#include <glm/glm.hpp>

#include <QtCore/QDebug>
#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QAbstractNativeEventFilter>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>

#include <Windows.h>
#include <spwdata.h>
#include <si.h>
#include <siapp.h>

using glm::ivec3;
using glm::vec3;

inline QDebug& operator<<(QDebug& dbg, const ivec3& c) {
    dbg.nospace() << "{type='ivec3'"
        ", x=" << c[0] <<
        ", y=" << c[1] <<
        ", z=" << c[2] <<
        "}";
    return dbg;
}

inline QDebug& operator<<(QDebug& dbg, const vec3& c) {
    dbg.nospace() << "{type='vec3'"
        ", x=" << QString::number(c[0], 'f', 2) <<
        ", y=" << QString::number(c[1], 'f', 2) <<
        ", z=" << QString::number(c[2], 'f', 2) <<
        "}";
    return dbg;
}


class SpacemouseWindow : public QWindow, public QAbstractNativeEventFilter {
public:

    SpacemouseWindow() : QWindow() {
        auto siInitResult = SiInitialize();

        auto deviceCount = SiGetNumDevices();

        SiOpenWinInit(&_siOpenData, reinterpret_cast<HWND>(winId()));
        _handle = SiOpen("interface", SI_ANY_DEVICE, SI_NO_MASK, SI_EVENT, &_siOpenData);
        qDebug() << "Hello" << siInitResult << " " << deviceCount;

    }

    void onMotionEvent(const SiSpwEvent& event) {
        const vec3 SCALE(2048);
        vec3 tr = vec3(event.u.spwData.mData[0], event.u.spwData.mData[1], event.u.spwData.mData[2]) / SCALE;
        vec3 ro = vec3(event.u.spwData.mData[3], event.u.spwData.mData[4], event.u.spwData.mData[5]) / SCALE;
        tr = glm::clamp(tr, -1.0f, 1.0f);
        ro = glm::clamp(ro, -1.0f, 1.0f);
        auto period = event.u.spwData.period;
        qDebug() << "TR" << tr;
        qDebug() << "RO" << ro;
    }

    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override {
        Q_UNUSED(eventType);
        MSG *msg = static_cast<MSG*>(message);
        static SiGetEventData eData;
        static SiSpwEvent siEvent;    /* 3DxWare data event */
        SiGetEventWinInit(&eData, msg->message, msg->wParam, msg->lParam);
        if (SiGetEvent(_handle, 0, &eData, &siEvent) == SI_IS_EVENT) {
            switch (siEvent.type) {
                case SI_MOTION_EVENT:
                    onMotionEvent(siEvent);
                    qDebug() << "SI_MOTION_EVENT";
                    break;

                case SI_BUTTON_EVENT:
                    qDebug() << "SI_BUTTON_EVENT";
                    break;

                case SI_ZERO_EVENT:
                    qDebug() << "SI_ZERO_EVENT";
                    break;

                case SI_APP_EVENT:
                    qDebug() << "SI_APP_EVENT";
                    break;

                case SI_CMD_EVENT:
                    qDebug() << "SI_CMD_EVENT";
                    break;

                case SI_DEVICE_CHANGE_EVENT:
                    qDebug() << "SI_DEVICE_CHANGE_EVENT";
                    break;

                default:
                    break;
            }
            return true;
        }

        return false;
    }

private:
    SiHdl _handle{ nullptr };
    SiOpenData _siOpenData;
};


class Qt59TestApp : public QGuiApplication {
public:
    Qt59TestApp(int argc, char* argv[]);
    ~Qt59TestApp();

private:
    SpacemouseWindow* _window { nullptr };
    void finish(int exitCode);
};



Qt59TestApp::Qt59TestApp(int argc, char* argv[]) :
    QGuiApplication(argc, argv) {
    _window = new SpacemouseWindow();
    installNativeEventFilter(_window);
}

Qt59TestApp::~Qt59TestApp() {
}

void Qt59TestApp::finish(int exitCode) {
}


int main(int argc, char * argv[]) {
    QCoreApplication::setApplicationName("Qt59Test");

    Qt59TestApp app(argc, argv);

    return app.exec();
}

