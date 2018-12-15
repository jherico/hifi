//
//  Created by Bradley Austin Davis on 2018/10/21
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderThread.h"

#include <mutex>

#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>

#include <QtCore/QFileInfo>
#include <QtGui/QWindow>
#include <QtGui/QImageReader>

#include <gl/QOpenGLContextWrapper.h>
#include <gpu/FrameIO.h>
#include <gpu/Texture.h>

#if OCULUS_MOBILE
#include <VrApi_Types.h>
#include <ovr/VrHandler.h>
#include <ovr/Helpers.h>

#include <VrApi.h>
#include <VrApi_Input.h>

static JNIEnv* _env { nullptr };
static JavaVM* _vm { nullptr };
static jobject _activity { nullptr };

struct HandController{
    ovrInputTrackedRemoteCapabilities caps;
    ovrInputStateTrackedRemote state;
    ovrResult stateResult{ ovrSuccess };
    ovrTracking tracking;
    ovrResult trackingResult{ ovrSuccess };

    void update(ovrMobile* session, double time = 0.0) {
        const auto& deviceId = caps.Header.DeviceID;
        stateResult = vrapi_GetCurrentInputState(session, deviceId, &state.Header);
        trackingResult = vrapi_GetInputTrackingState(session, deviceId, 0.0, &tracking);
    }
};

std::vector<HandController> devices;

extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *, void *) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ", __FUNCTION__);
    return JNI_VERSION_1_6;
}


JNIEXPORT void JNICALL Java_io_highfidelity_frameplayer_FramePlayerActivity_nativeOnCreate(JNIEnv* env, jobject obj) {
    env->GetJavaVM(&_vm);
    _activity = env->NewGlobalRef(obj);
}
}

#else

static std::mutex windowLock;
static ANativeWindow* window = nullptr;
using NativeWindowTask = std::function<void(ANativeWindow*&)>;

void withNativeWindow(const NativeWindowTask& task) {
    std::unique_lock<std::mutex> lock(windowLock);
    task(window);
}

static std::mutex touchLock;
glm::vec2 touch{ 0 };
int lastAction = 1;
using TouchTask = std::function<void(int action, const glm::vec2&)>;

void updateTouch(int action, float x, float y) {
    std::unique_lock<std::mutex> lock(touchLock);
    touch = { x , y };
    lastAction = action;
}

void withTouch(const TouchTask& task) {
    std::unique_lock<std::mutex> lock(touchLock);
    task(lastAction, {touch.x, touch.y});
}

extern "C" {

JNIEXPORT void JNICALL Java_io_highfidelity_frameplayer_FramePlayerActivity_nativeOnCreate(JNIEnv* env, jobject obj) {
}

JNIEXPORT jlong JNICALL Java_io_highfidelity_frameplayer_RenderActivity_nativeOnCreate(JNIEnv* env, jobject obj) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ_JNI", __FUNCTION__);
    return 0;
}

JNIEXPORT void JNICALL Java_io_highfidelity_frameplayer_RenderActivity_nativeOnDestroy(JNIEnv* env, jobject obj, jlong handle) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ_JNI", __FUNCTION__);
}

JNIEXPORT void JNICALL Java_io_highfidelity_frameplayer_RenderActivity_nativeOnResume(JNIEnv* env, jobject obj, jlong handle) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ_JNI", __FUNCTION__);
}

JNIEXPORT void JNICALL Java_io_highfidelity_frameplayer_RenderActivity_nativeOnPause(JNIEnv* env, jobject obj, jlong handle) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ_JNI", __FUNCTION__);
}

JNIEXPORT void JNICALL Java_io_highfidelity_frameplayer_RenderActivity_nativeOnTouch(JNIEnv* env, jobject obj, jlong action, jfloat x, jfloat y) {
    updateTouch(action, x, y);
}

JNIEXPORT void JNICALL Java_io_highfidelity_frameplayer_RenderActivity_nativeOnSurfaceCreated(JNIEnv* env, jobject obj, jlong handle, jobject surface) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ_JNI", __FUNCTION__);
    withNativeWindow([&](ANativeWindow*& window){
        window = ANativeWindow_fromSurface( env, surface );
    });
}

JNIEXPORT void JNICALL Java_io_highfidelity_frameplayer_RenderActivity_nativeOnSurfaceChanged(JNIEnv* env, jobject obj, jlong handle, jobject surface) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ_JNI", __FUNCTION__);
    withNativeWindow([&](ANativeWindow*& window){
        window = ANativeWindow_fromSurface( env, surface );
    });
}

JNIEXPORT void JNICALL Java_io_highfidelity_frameplayer_RenderActivity_nativeOnSurfaceDestroyed(JNIEnv* env, jobject obj, jlong handle) {
    __android_log_write(ANDROID_LOG_WARN, "QQQ_JNI", __FUNCTION__);
    withNativeWindow([&](ANativeWindow*& window){
        window = nullptr;
    });
}

} // extern "C"
#endif

static const char* FRAME_FILE = "assets:/frames/20181124_1047.json";

static void textureLoader(const std::string& filename, const gpu::TexturePointer& texture, uint16_t layer) {
    QImage image;
    QImageReader(filename.c_str()).read(&image);
    if (layer > 0) {
        return;
    }
    texture->assignStoredMip(0, image.byteCount(), image.constBits());
}

void RenderThread::submitFrame(const gpu::FramePointer& frame) {
    std::unique_lock<std::mutex> lock(_frameLock);
    _pendingFrames.push(frame);
}

void RenderThread::move(const glm::vec3& v) {
    std::unique_lock<std::mutex> lock(_frameLock);
    _correction = glm::inverse(glm::translate(mat4(), v)) * _correction;
}

void RenderThread::ensureSurface(ANativeWindow* nativeWindow) {
    if (_nativeWindow != nativeWindow) {
        if (nativeWindow) {
            _glContext.createSurface(nativeWindow, _size);
        } else {
            _glContext.createSurface();
        }
        _nativeWindow = nativeWindow;
        _glContext.makeCurrent();
    }
}

static void* getGlProcessAddress(const char *namez) {
    auto result = eglGetProcAddress(namez);
    return (void*)result;
}

void RenderThread::initialize(QWindow* window) {
    if (QFileInfo(FRAME_FILE).exists()) {
        auto frame = gpu::readFrame(FRAME_FILE, _externalTexture, &textureLoader);
        submitFrame(frame);
    }

    std::unique_lock<std::mutex> lock(_frameLock);
    setObjectName("RenderThread");
    Parent::initialize();
    _size = window->size();
    _glContext.create();
    _glContext.createSurface();
    //move({ 0, 0, -100 });

    withValidSurface([&]{
        bool currentResult = _glContext.makeCurrent();
        if (currentResult) {
            gladLoadGLES2Loader(getGlProcessAddress);
            glGenTextures(1, &_externalTexture);
            glBindTexture(GL_TEXTURE_2D, _externalTexture);
            static const glm::u8vec4 color{ 0 };
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color);
            gl::setSwapInterval(0);
        }

        // GPU library init
        gpu::Context::init<gpu::gl::GLBackend>();
        _glContext.makeCurrent();
        _gpuContext = std::make_shared<gpu::Context>();
        _backend = _gpuContext->getBackend();
        _glContext.doneCurrent();
    });
    _thread->setObjectName("RenderThread");
}

void RenderThread::withValidSurface(const Task& task) {
    withNativeWindow([&](ANativeWindow*& nativeWindow) {
        ensureSurface(nativeWindow);
        task();
    });
}

void RenderThread::setup() {
    // Wait until the context has been moved to this thread
    { std::unique_lock<std::mutex> lock(_frameLock); }
    _gpuContext->beginFrame();
    _gpuContext->endFrame();



#if OCULUS_MOBILE
    _glContext.makeCurrent();
    ovr::VrHandler::initVr();
    _vm->AttachCurrentThread(&_env, nullptr);
    jclass cls = _env->GetObjectClass(_activity);
    jmethodID mid = _env->GetMethodID(cls, "launchOculusActivity", "()V");
    _env->CallVoidMethod(_activity, mid);
    ovr::VrHandler::setHandler(this);
#endif
}

void RenderThread::shutdown() {
    _activeFrame.reset();
    while (!_pendingFrames.empty()) {
        _gpuContext->consumeFrameUpdates(_pendingFrames.front());
        _pendingFrames.pop();
    }
    _gpuContext->shutdown();
    _gpuContext.reset();
}

void RenderThread::handleInput() {
#if OCULUS_MOBILE
    auto readResult = ovr::VrHandler::withOvrMobile([&](ovrMobile *session) {
        for (auto &controller : devices) {
            controller.update(session);
        }
    });

    if (readResult) {
        for (auto &controller : devices) {
            const auto &caps = controller.caps;
            if (controller.stateResult >= 0) {
                const auto &remote = controller.state;
                if (remote.Joystick.x != 0.0f || remote.Joystick.y != 0.0f) {
                    glm::vec3 translation;
                    float rotation = 0.0f;
                    if (caps.ControllerCapabilities & ovrControllerCaps_LeftHand) {
                        translation = glm::vec3{0.0f, -remote.Joystick.y, 0.0f};
                    } else {
                        translation = glm::vec3{remote.Joystick.x, 0.0f, -remote.Joystick.y};
                    }
                    float scale = 0.1f + (1.9f * remote.GripTrigger);
                    _correction = glm::translate(glm::mat4(), translation * scale) * _correction;
                }
            }
        }
    }
#else
    withTouch([&](int action, const glm::vec2& v){
        static int lastAction = 1;
        if (action == 1) {
            lastAction = 1;
            return;
        }

        auto nv = (v * 2.0f) - 1.0f;
        //nv.y *= -1.0f;
        static glm::vec2 start = nv;
        if (lastAction == 1) {
            start = nv;
        }

        lastAction = action;
        nv = nv - start;
        qWarning() << "QQQ" << __FUNCTION__ << action << nv.x << nv.y;

        glm::vec3 translation { nv.x, 0.0f, nv.y};
        float scale = 10.1f;
        _correction = glm::translate(glm::mat4(), translation * scale) * _correction;
    });
#endif
}

void RenderThread::renderFrame(gpu::FramePointer& frame) {
    auto& eyeProjections = _activeFrame->stereoState._eyeProjections;
    auto& eyeOffsets = _activeFrame->stereoState._eyeViews;
#if OCULUS_MOBILE
    const auto& tracking = beginFrame();
    // Quest
    auto frameCorrection = _correction * ovr::toGlm(tracking.HeadPose.Pose);
    _backend->setCameraCorrection(glm::inverse(frameCorrection), _activeFrame->view);
    ovr::for_each_eye([&](ovrEye eye){
        const auto& eyeInfo = tracking.Eye[eye];
        eyeProjections[eye] = ovr::toGlm(eyeInfo.ProjectionMatrix);
        eyeOffsets[eye] = ovr::toGlm(eyeInfo.ViewMatrix);
    });

    static std::once_flag once;
    std::call_once(once, [&]{
        withOvrMobile([&](ovrMobile* session){
            int deviceIndex = 0;
            ovrInputCapabilityHeader capsHeader;
            while (vrapi_EnumerateInputDevices(session, deviceIndex, &capsHeader) >= 0) {
                if (capsHeader.Type == ovrControllerType_TrackedRemote) {
                    HandController controller = {};
                    controller.caps.Header = capsHeader;
                    controller.state.Header.ControllerType = ovrControllerType_TrackedRemote;
                    vrapi_GetInputDeviceCapabilities( session, &controller.caps.Header);
                    devices.push_back(controller);
                }
                ++deviceIndex;
            }
        });
    });
    _gpuContext->enableStereo(true);
#else
    _backend->setCameraCorrection(glm::inverse(_correction), _activeFrame->view);

    _activeFrame->stereoState._enable = false;
    eyeProjections[0][0] = vec4{ 0.91729, 0.0, -0.17407, 0.0 };
    eyeProjections[0][1] = vec4{ 0.0, 0.083354, -0.106141, 0.0 };
    eyeProjections[0][2] = vec4{ 0.0, 0.0, -1.0, -0.2 };
    eyeProjections[0][3] = vec4{ 0.0, 0.0, -1.0, 0.0 };
    eyeProjections[1][0] = vec4{ 0.91729, 0.0, 0.17407, 0.0 };
    eyeProjections[1][1] = vec4{ 0.0, 0.083354, -0.106141, 0.0 };
    eyeProjections[1][2] = vec4{ 0.0, 0.0, -1.0, -0.2 };
    eyeProjections[1][3] = vec4{ 0.0, 0.0, -1.0, 0.0 };
    eyeOffsets[0][3] = vec4{ -0.0327499993, 0.0, -0.0149999997, 1.0 };
    eyeOffsets[1][3] = vec4{ 0.0327499993, 0.0, -0.0149999997, 1.0 };
#endif
    _glContext.makeCurrent();
    _backend->recycle();
    _backend->syncCache();
    if (frame && !frame->batches.empty()) {
        _gpuContext->executeFrame(frame);
    }
    auto& glbackend = (gpu::gl::GLBackend&)(*_backend);
    glm::uvec2 fboSize{ frame->framebuffer->getWidth(), frame->framebuffer->getHeight() };

#if OCULUS_MOBILE
    auto finalTexture = glbackend.getTextureID(frame->framebuffer->getRenderBuffer(0));
    _glContext.doneCurrent();
    presentFrame(finalTexture, fboSize, tracking);
#else
    const auto& windowSize = _size;
    auto finalFbo = glbackend.getFramebufferID(frame->framebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, finalFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBlitFramebuffer(
        0, 0, fboSize.x, fboSize.y,
        0, 0, windowSize.width(), windowSize.height(),
        GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    CHECK_GL_ERROR();
#endif
}

bool RenderThread::process() {
#if OCULUS_MOBILE
    pollTask();

    if (!vrActive()) {
        QThread::msleep(1);
        return true;
    }
#endif

    std::queue<gpu::FramePointer> pendingFrames;
    {
        std::unique_lock<std::mutex> lock(_frameLock);
        pendingFrames.swap(_pendingFrames);
    }

    while (!pendingFrames.empty()) {
        _activeFrame = pendingFrames.front();
        pendingFrames.pop();
        _gpuContext->consumeFrameUpdates(_activeFrame);
        _activeFrame->stereoState._enable = true;
    }

    withValidSurface([&]{
        if (!_activeFrame) {
            glClearColor(1, 0, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            CHECK_GL_ERROR();
            QThread::msleep(1);
        } else {
            handleInput();
            renderFrame(_activeFrame);
        }
        _glContext.swapBuffers();
    });
    return true;
}
