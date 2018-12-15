//
//  Created by Bradley Austin Davis on 2018/11/15
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Context.h"

#include <array>
#include <vector>
#include <mutex>

#include <QtCore/QDebug>
#include <QtGui/QWindow>
#include <QtGui/QOpenGLDebugMessage>

#include <EGL/eglext.h>

#include "../gl/Config.h"
#include "../gl/GLHelpers.h"
#include "../gl/GLLogging.h"

#if !defined(EGL_OPENGL_ES3_BIT_KHR)
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

using namespace egl;

static void APIENTRY debugMessageCallback(GLenum source,
                                   GLenum type,
                                   GLuint id,
                                   GLenum severity,
                                   GLsizei length,
                                   const GLchar* message,
                                   const void* userParam) {
    QOpenGLDebugMessage::Type qtype = QOpenGLDebugMessage::InvalidType;
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            qtype = QOpenGLDebugMessage::ErrorType;
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            qtype = QOpenGLDebugMessage::DeprecatedBehaviorType;
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            qtype = QOpenGLDebugMessage::UndefinedBehaviorType;
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            qtype = QOpenGLDebugMessage::PortabilityType;
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            qtype = QOpenGLDebugMessage::PerformanceType;
            break;
        case GL_DEBUG_TYPE_OTHER:
            qtype = QOpenGLDebugMessage::OtherType;
            break;
        case GL_DEBUG_TYPE_MARKER:
            qtype = QOpenGLDebugMessage::MarkerType;
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            qtype = QOpenGLDebugMessage::GroupPushType;
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            qtype = QOpenGLDebugMessage::GroupPopType;
            break;
        default:
            break;
    };

    QOpenGLDebugMessage::Severity sev = QOpenGLDebugMessage::InvalidSeverity;
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            sev = QOpenGLDebugMessage::HighSeverity;
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            sev = QOpenGLDebugMessage::MediumSeverity;
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            sev = QOpenGLDebugMessage::NotificationSeverity;
            break;
        case GL_DEBUG_SEVERITY_LOW:
            sev = QOpenGLDebugMessage::LowSeverity;
            break;
        default:
            break;
    }

    qCWarning(glLogging) << QOpenGLDebugMessage::createApplicationMessage(message, id, sev, qtype );
}

EGLConfig Context::findConfig(EGLDisplay display) {
    // Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
    // flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
    // settings, and that is completely wasted for our warp target.
    std::vector<EGLConfig> configs;
    {
        const int MAX_CONFIGS = 1024;
        EGLConfig configsBuffer[MAX_CONFIGS];
        EGLint numConfigs = 0;
        if (eglGetConfigs(display, configsBuffer, MAX_CONFIGS, &numConfigs) == EGL_FALSE) {
            qCWarning(glLogging) << "Failed to fetch configs";
            return 0;
        }
        configs.resize(numConfigs);
        memcpy(configs.data(), configsBuffer, sizeof(EGLConfig) * numConfigs);
    }

    std::vector<std::pair<EGLint, EGLint>> configAttribs{
        { EGL_RED_SIZE, 8 },   { EGL_GREEN_SIZE, 8 },   { EGL_BLUE_SIZE, 8 }, { EGL_ALPHA_SIZE, 8 },
        { EGL_DEPTH_SIZE, 0 }, { EGL_STENCIL_SIZE, 0 }, { EGL_SAMPLES, 0 },
    };

    auto matchAttrib = [&](EGLConfig config, const std::pair<EGLint, EGLint>& attribAndValue) {
        EGLint value = 0;
        eglGetConfigAttrib(display, config, attribAndValue.first, &value);
        return (attribAndValue.second == value);
    };

    auto matchAttribFlags = [&](EGLConfig config, const std::pair<EGLint, EGLint>& attribAndValue) {
        EGLint value = 0;
        eglGetConfigAttrib(display, config, attribAndValue.first, &value);
        return (value & attribAndValue.second) == attribAndValue.second;
    };

    auto matchConfig = [&](EGLConfig config) {
        if (!matchAttribFlags(config, { EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR})) {
            return false;
        }
        // The pbuffer config also needs to be compatible with normal window rendering
        // so it can share textures with the window context.
        if (!matchAttribFlags(config, { EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT})) {
            return false;
        }

        for (const auto& attrib : configAttribs) {
            if (!matchAttrib(config, attrib)) {
                return false;
            }
        }

        return true;
    };


    for (const auto& config : configs) {
        if (matchConfig(config)) {
            return config;
        }
    }

    return 0;
}

Context::Context() {

}

Context::~Context() {
    destroy();
}

bool Context::create(EGLContext shareContext) {
    static std::once_flag once;
    if (display == EGL_NO_DISPLAY) {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    std::call_once(once, [&]{
        EGLint majorVersion;
        EGLint minorVersion;
        eglInitialize(display, &majorVersion, &minorVersion);
    });

    if (config == 0) {
        config = findConfig(display);
    }

    if (config == 0) {
        qCWarning(glLogging) << "Failed eglChooseConfig";
        return false;
    }

    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_CONTEXT_OPENGL_NO_ERROR_KHR, EGL_TRUE,
        EGL_NONE
    };

    context = eglCreateContext(display, config, shareContext, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        qCWarning(glLogging) << "Failed eglCreateContext";
        return false;
    }
    return true;
}

void Context::clearSurface() {
    if (surface != EGL_NO_SURFACE) {
        doneCurrent();
        eglDestroySurface(display, surface);
        surface = EGL_NO_SURFACE;
    }
}

bool Context::createSurface() {
    if (context == EGL_NO_CONTEXT) {
        return false;
    }
    clearSurface();
    const EGLint surfaceAttribs[] = { EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE };
    surface = eglCreatePbufferSurface(display, config, surfaceAttribs);
    if (surface == EGL_NO_SURFACE) {
        qCWarning(glLogging) << "Failed eglCreatePbufferSurface";
        return false;
    }
    return true;
}

bool Context::createSurface(ANativeWindow* window, const QSize& size) {
    if (context == EGL_NO_CONTEXT) {
        return false;
    }
    clearSurface();
    auto eglErr = eglGetError();
    surface = eglCreateWindowSurface(display, config, window, nullptr);
    if (surface == EGL_NO_SURFACE) {
        eglErr = eglGetError();
        qCWarning(glLogging) << "Failed eglCreateWindowSurface";
        return false;
    }
    return true;
}

bool Context::makeCurrent() {
    if (context == EGL_NO_CONTEXT || surface == EGL_NO_SURFACE) {
        return false;
    }
    gl::initModuleGl();

    return eglMakeCurrent(display, surface, surface, context) != EGL_FALSE;
}

bool Context::swapBuffers() {
    bool result = eglSwapBuffers(display, surface) != EGL_FALSE;
    return result;
}

void Context::doneCurrent() {
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void Context::destroy() {
    if (context != EGL_NO_CONTEXT) {
        eglDestroyContext(display, context);
        context = EGL_NO_CONTEXT;
    }

    if (surface != EGL_NO_SURFACE) {
        eglDestroySurface(display, surface);
        surface = EGL_NO_SURFACE;
    }
}
