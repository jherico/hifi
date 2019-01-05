//
//  Created by Bradley Austin Davis on 2018/11/15
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "VrHandler.h"

#include <android/log.h>
#include <android/native_window_jni.h>
#include <unistd.h>

#include <VrApi.h>
#include <VrApi_Helpers.h>
#include <VrApi_Types.h>
//#include <OVR_Platform.h>

#include "GLContext.h"
#include "Helpers.h"
#include "Framebuffer.h"
#include "OculusMobileActivity.h"

using namespace ovr;

static thread_local bool isRenderThread { false };

struct VrSurface : public TaskQueue {
    using HandlerTask = VrHandler::HandlerTask;
    VrHandler *handler{nullptr};
    ovrMobile *session{nullptr};
    ANativeWindow* nativeWindow{ nullptr };
    bool resumed { false };
    GLContext vrglContext;
    Framebuffer eyeFbos[2];
    uint32_t readFbo{0};
    jobject oculusActivity{ nullptr };
    JavaVM* vm{nullptr};
    uint32_t presentIndex{0};
    double displayTime{0};
    static constexpr float EYE_BUFFER_SCALE = 0.75f;

    void onCreate(JNIEnv* env, jobject activity) {
        env->GetJavaVM(&vm);
        oculusActivity = env->NewGlobalRef(activity);
    }

    void setResumed(bool newResumed) {
        this->resumed = newResumed;
        submitRenderThreadTask([this](VrHandler* handler){ updateVrMode(); });
    }

    void setNativeWindow(ANativeWindow* newNativeWindow) {
        auto oldNativeWindow = nativeWindow;
        nativeWindow = newNativeWindow;
        if (oldNativeWindow) {
            ANativeWindow_release(oldNativeWindow);
        }
        submitRenderThreadTask([this](VrHandler* handler){ updateVrMode(); });
    }

    void init() {
        if (!handler) {
            return;
        }

        vrapi_SetPerfThread(session, VRAPI_PERF_THREAD_TYPE_MAIN, pthread_self());

        EGLContext currentContext = eglGetCurrentContext();
        EGLDisplay currentDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        vrglContext.create(currentDisplay, currentContext);
        vrglContext.makeCurrent();

        glm::uvec2 eyeTargetSize;
        withEnv([&](JNIEnv* env){
            ovrJava java{ vm, env, oculusActivity };
            eyeTargetSize = glm::uvec2 {
                vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH) * EYE_BUFFER_SCALE,
                vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT) * EYE_BUFFER_SCALE,
            };
        });
        __android_log_print(ANDROID_LOG_WARN, "QQQ_OVR", "QQQ Eye Size %d, %d", eyeTargetSize.x, eyeTargetSize.y);
        ovr::for_each_eye([&](ovrEye eye) {
            eyeFbos[eye].create(eyeTargetSize);
        });
        glGenFramebuffers(1, &readFbo);
        vrglContext.doneCurrent();
    }

    void shutdown() {
    }

    void setHandler(VrHandler *newHandler) {
        withLock([&] {
            isRenderThread = newHandler != nullptr;
            if (handler != newHandler) {
                shutdown();
                handler = newHandler;
                init();
                if (handler) {
                    updateVrMode();
                }
            }
        });
    }

    void submitRenderThreadTask(const HandlerTask &task) {
        withLockConditional([&](Lock &lock) {
            if (handler != nullptr) {
                submitTaskBlocking(lock, [&] {
                    task(handler);
                });
            }
        });
    }

    void withEnv(const std::function<void(JNIEnv*)>& f) {
        JNIEnv* env = nullptr;
        bool attached = false;
        vm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (!env) {
            attached = true;
            vm->AttachCurrentThread(&env, nullptr);
        }
        f(env);
        if (attached) {
            vm->DetachCurrentThread();
        }
    }

    void updateVrMode() {
        // For VR mode to be valid, the activity must be between an onResume and
        // an onPause call and must additionally have a valid native window handle
        bool vrReady = resumed && nullptr != nativeWindow;
        // If we're IN VR mode, we'll have a non-null ovrMobile pointer in session
        bool vrRunning = session != nullptr;
        if (vrReady != vrRunning) {
            if (vrRunning) {
                __android_log_write(ANDROID_LOG_WARN, "QQQ_OVR", "vrapi_LeaveVrMode");
                vrapi_LeaveVrMode(session);
                session = nullptr;
                oculusActivity = nullptr;
            } else {
                __android_log_write(ANDROID_LOG_WARN, "QQQ_OVR", "vrapi_EnterVrMode");
                withEnv([&](JNIEnv* env){
                    ovrJava java{ vm, env, oculusActivity };
                    ovrModeParms modeParms = vrapi_DefaultModeParms(&java);
                    modeParms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
                    modeParms.Display = (unsigned long long) vrglContext.display;
                    modeParms.ShareContext = (unsigned long long) vrglContext.context;
                    modeParms.WindowSurface = (unsigned long long) nativeWindow;
                    session = vrapi_EnterVrMode(&modeParms);
                    ovrPosef trackingTransform = vrapi_GetTrackingTransform( session, VRAPI_TRACKING_TRANSFORM_SYSTEM_CENTER_EYE_LEVEL);
                    vrapi_SetTrackingTransform( session, trackingTransform );
                    vrapi_SetPerfThread(session, VRAPI_PERF_THREAD_TYPE_RENDERER, pthread_self());
                });
            }
        }
    }

    void presentFrame(uint32_t sourceTexture, const glm::uvec2 &sourceSize, const ovrTracking2& tracking) {
        vrglContext.makeCurrent();
        ovrLayerProjection2 layer = vrapi_DefaultLayerProjection2();
        layer.HeadPose = tracking.HeadPose;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
        glFramebufferTexture(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, sourceTexture, 0);
        GLenum framebufferStatus = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
        if (GL_FRAMEBUFFER_COMPLETE != framebufferStatus) {
            __android_log_print(ANDROID_LOG_WARN, "QQQ_OVR", "incomplete framebuffer");
        }
        ovr::for_each_eye([&](ovrEye eye) {
            const auto &eyeTracking = tracking.Eye[eye];
            auto &eyeFbo = eyeFbos[eye];
            const auto &destSize = eyeFbo._size;
            eyeFbo.bind();
            CHECK_GL_ERROR();
            GLenum framebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
            if (GL_FRAMEBUFFER_COMPLETE != framebufferStatus) {
                __android_log_print(ANDROID_LOG_WARN, "QQQ_OVR", "Bad framebuffer Status");
            }
            glClearColor((eye == VRAPI_EYE_LEFT) ? 1.0f : 0.0f,
                         (eye == VRAPI_EYE_LEFT) ? 0.0f : 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            CHECK_GL_ERROR();
            auto sourceWidth = sourceSize.x / 2;
            auto sourceX = (eye == VRAPI_EYE_LEFT) ? 0 : sourceWidth;
            glBlitFramebuffer(
                sourceX, 0, sourceX + sourceWidth, sourceSize.y,
                0, 0, destSize.x, destSize.y,
                GL_COLOR_BUFFER_BIT, GL_NEAREST);
            eyeFbo.updateLayer(eye, layer, &eyeTracking.ProjectionMatrix);
            eyeFbo.advance();
        });
        glFramebufferTexture(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0);
        glFlush();

        ovrLayerHeader2 *layerHeader = &layer.Header;
        ovrSubmitFrameDescription2 frameDesc = {};
        frameDesc.FrameIndex = presentIndex;
        frameDesc.DisplayTime = displayTime;
        frameDesc.LayerCount = 1;
        frameDesc.Layers = &layerHeader;
        vrapi_SubmitFrame2(session, &frameDesc);
        ++presentIndex;
        vrglContext.doneCurrent();
    }

    ovrTracking2 beginFrame() {
        displayTime = vrapi_GetPredictedDisplayTime(session, presentIndex);
        return vrapi_GetPredictedTracking2(session, displayTime);
    }
};

static VrSurface SURFACE;

bool VrHandler::vrActive() const {
    return SURFACE.session != nullptr;
}

void VrHandler::setHandler(VrHandler* handler) {
    SURFACE.setHandler(handler);
}

void VrHandler::submitRenderThreadTask(const HandlerTask& task) {
    SURFACE.submitRenderThreadTask(task);
}

void VrHandler::pollTask() {
    SURFACE.pollTask();
}

void VrHandler::onCreate(JNIEnv *env, jobject activity) {
    SURFACE.onCreate(env, activity);
}

void VrHandler::makeCurrent() {
    if (!SURFACE.vrglContext.makeCurrent()) {
        __android_log_write(ANDROID_LOG_WARN, "QQQ", "Failed to make GL current");
    }
}

void VrHandler::doneCurrent() {
    SURFACE.vrglContext.doneCurrent();
}

ovrTracking2 VrHandler::beginFrame() {
    return SURFACE.beginFrame();
}

void VrHandler::presentFrame(uint32_t sourceTexture, const glm::uvec2 &sourceSize, const ovrTracking2& tracking) const {
    SURFACE.presentFrame(sourceTexture, sourceSize, tracking);
}

bool VrHandler::withOvrJava(const OvrJavaTask& task) {
    SURFACE.withEnv([&](JNIEnv* env){
        ovrJava java{ SURFACE.vm, env, SURFACE.oculusActivity };
        task(&java);
    });
    return true;
}

bool VrHandler::withOvrMobile(const OvrMobileTask &task) {
    auto sessionTask = [&]()->bool{
        if (!SURFACE.session) {
            return false;
        }
        task(SURFACE.session);
        return true;
    };

    if (isRenderThread) {
        return sessionTask();
    }

    bool result = false;
    SURFACE.withLock([&]{
        result = sessionTask();
    });
    return result;
}


void VrHandler::initVr(const char* appId) {
    withOvrJava([&](const ovrJava* java){
        ovrInitParms initParms = vrapi_DefaultInitParms(java);
        initParms.GraphicsAPI = VRAPI_GRAPHICS_API_OPENGL_ES_3;
        if (vrapi_Initialize(&initParms) != VRAPI_INITIALIZE_SUCCESS) {
            __android_log_write(ANDROID_LOG_WARN, "QQQ_OVR", "Failed vrapi init");
        }
    });

  //  if (appId) {
  //      auto platformInitResult = ovr_PlatformInitializeAndroid(appId, activity.object(), env);
  //      if (ovrPlatformInitialize_Success != platformInitResult) {
  //          __android_log_write(ANDROID_LOG_WARN, "QQQ_OVR", "Failed ovr platform init");
  //      }
  //  }
}

void VrHandler::shutdownVr() {
    vrapi_Shutdown();
}


void VrHandler::setResumed(bool resumed) {
    SURFACE.setResumed(resumed);
}

void VrHandler::setNativeWindow(ANativeWindow *window) {
    SURFACE.setNativeWindow(window);
}

