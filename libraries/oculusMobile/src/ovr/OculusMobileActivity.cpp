    //
//  Created by Bradley Austin Davis on 2018/11/15
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusMobileActivity.h"

#include <atomic>

#include <android/log.h>
#include <android/native_window_jni.h>

#include <VrApi.h>

using namespace ovr;

void OculusMobileActivity::setNativeWindow(ANativeWindow *newNativeWindow) {
    VrHandler::setNativeWindow(newNativeWindow);
}

void OculusMobileActivity::releaseNativeWindow() {
    VrHandler::setNativeWindow(nullptr);
}

void OculusMobileActivity::nativeOnCreate(JNIEnv *env, jobject obj) {
    VrHandler::onCreate(env, obj);
}

void OculusMobileActivity::nativeOnDestroy(JNIEnv *env) {
}

void OculusMobileActivity::nativeOnResume(JNIEnv *env) {
    VrHandler::setResumed(true);
}

void OculusMobileActivity::nativeOnPause(JNIEnv *env) {
    VrHandler::setResumed(false);
}

void OculusMobileActivity::nativeSurfaceCreated(ANativeWindow *newNativeWindow) {
    setNativeWindow(newNativeWindow);
}

void OculusMobileActivity::nativeSurfaceChanged(ANativeWindow *newNativeWindow) {
    setNativeWindow(newNativeWindow);
}

void OculusMobileActivity::nativeSurfaceDestroyed() {
    releaseNativeWindow();
}
