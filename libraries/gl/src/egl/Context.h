//
//
//  Created by Bradley Austin Davis on 2018/11/15
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <memory>

#include <EGL/egl.h>
#include <glad/glad.h>

class QWindow;
class QSize;

namespace egl {

struct Context {
    using Pointer = std::shared_ptr<EGLContext>;
    EGLSurface surface{ EGL_NO_SURFACE };
    EGLContext context{ EGL_NO_CONTEXT };

    EGLDisplay display{ EGL_NO_DISPLAY };
    EGLConfig config { 0 };

    Context();
    ~Context();
    static EGLConfig findConfig(EGLDisplay display);
    bool makeCurrent();
    bool swapBuffers();
    void doneCurrent();
    bool create(EGLContext shareContext = EGL_NO_CONTEXT);
    bool createSurface();
    bool createSurface(ANativeWindow* window, const QSize& size);
    void clearSurface();
//    bool create(QWindow* window, EGLContext shareContext = EGL_NO_CONTEXT);
//    bool create(ANativeWindow* window, const QSize& size, EGLContext shareContext = EGL_NO_CONTEXT);
    void destroy();
    operator bool() const { return context != EGL_NO_CONTEXT; }
};

}
