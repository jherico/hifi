//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenVrDisplayPlugin.h"

// Odd ordering of header is required to avoid 'macro redinition warnings'
#include <AudioClient.h>

#include <QtCore/QThread>
#include <QtCore/QLoggingCategory>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>

#include <GLMHelpers.h>

#include <gl/Config.h>
#include <gl/Context.h>
#include <gl/GLShaders.h>

#include <gpu/Frame.h>
#include <gpu/gl/GLBackend.h>


#include <ViewFrustum.h>
#include <PathUtils.h>
#include <shared/NsightHelpers.h>
#include <controllers/Pose.h>
#include <display-plugins/CompositorHelper.h>
#include <ui-plugins/PluginContainer.h>
#include <gl/OffscreenGLCanvas.h>

#include "OpenVrHelpers.h"

Q_DECLARE_LOGGING_CATEGORY(displayplugins)

const char* OpenVrDisplayPlugin::NAME { "OpenVR (Vive)" };
const char* StandingHMDSensorMode { "Standing HMD Sensor Mode" }; // this probably shouldn't be hardcoded here
const char* OpenVrThreadedSubmit { "OpenVR Threaded Submit" }; // this probably shouldn't be hardcoded here

PoseData _nextRenderPoseData;
PoseData _nextSimPoseData;

#define MIN_CORES_FOR_NORMAL_RENDER 5
const bool forceInterleavedReprojection = (QThread::idealThreadCount() < MIN_CORES_FOR_NORMAL_RENDER);

static std::array<vr::Hmd_Eye, 2> VR_EYES { { vr::Eye_Left, vr::Eye_Right } };
bool _openVrDisplayActive { false };
// Flip y-axis since GL UV coords are backwards.
static vr::VRTextureBounds_t OPENVR_TEXTURE_BOUNDS_LEFT{ 0, 0, 0.5f, 1 };
static vr::VRTextureBounds_t OPENVR_TEXTURE_BOUNDS_RIGHT{ 0.5f, 0, 1, 1 };

bool OpenVrDisplayPlugin::isSupported() const {
    return openVrSupported();
}

glm::mat4 OpenVrDisplayPlugin::getEyeProjection(Eye eye, const glm::mat4& baseProjection) const {
    if (_system) {
        ViewFrustum baseFrustum;
        baseFrustum.setProjection(baseProjection);
        float baseNearClip = baseFrustum.getNearClip();
        float baseFarClip = baseFrustum.getFarClip();
        vr::EVREye openVrEye = (eye == Left) ? vr::Eye_Left : vr::Eye_Right;
        return toGlm(_system->GetProjectionMatrix(openVrEye, baseNearClip, baseFarClip));
    } else {
        return baseProjection;
    }
}

glm::mat4 OpenVrDisplayPlugin::getCullingProjection(const glm::mat4& baseProjection) const {
    if (_system) {
        ViewFrustum baseFrustum;
        baseFrustum.setProjection(baseProjection);
        float baseNearClip = baseFrustum.getNearClip();
        float baseFarClip = baseFrustum.getFarClip();
        // FIXME Calculate the proper combined projection by using GetProjectionRaw values from both eyes
        return toGlm(_system->GetProjectionMatrix((vr::EVREye)0, baseNearClip, baseFarClip));
    } else {
        return baseProjection;
    }
}

float OpenVrDisplayPlugin::getTargetFrameRate() const {

    uint32_t reprojectionFlags = 0;
    withNonPresentThreadLock([&] {
        reprojectionFlags = _frameTiming.m_nReprojectionFlags;
    });

    if (VRCompositor_ReprojectionAsync == (reprojectionFlags & VRCompositor_ReprojectionAsync)) {
        return TARGET_RATE_OpenVr / 2.0f;
    }

    return TARGET_RATE_OpenVr;
}

void OpenVrDisplayPlugin::init() {
    Plugin::init();

    _lastGoodHMDPose.m[0][0] = 1.0f;
    _lastGoodHMDPose.m[0][1] = 0.0f;
    _lastGoodHMDPose.m[0][2] = 0.0f;
    _lastGoodHMDPose.m[0][3] = 0.0f;
    _lastGoodHMDPose.m[1][0] = 0.0f;
    _lastGoodHMDPose.m[1][1] = 1.0f;
    _lastGoodHMDPose.m[1][2] = 0.0f;
    _lastGoodHMDPose.m[1][3] = 1.8f;
    _lastGoodHMDPose.m[2][0] = 0.0f;
    _lastGoodHMDPose.m[2][1] = 0.0f;
    _lastGoodHMDPose.m[2][2] = 1.0f;
    _lastGoodHMDPose.m[2][3] = 0.0f;

    emit deviceConnected(getName());
}

bool OpenVrDisplayPlugin::internalActivate() {
    if (!_system) {
        _system = acquireOpenVrSystem();
    }
    if (!_system) {
        qWarning() << "Failed to initialize OpenVR";
        return false;
    }

    // If OpenVR isn't running, then the compositor won't be accessible
    // FIXME find a way to launch the compositor?
    if (!vr::VRCompositor()) {
        qWarning() << "Failed to acquire OpenVR compositor";
        releaseOpenVrSystem();
        _system = nullptr;
        return false;
    }

    if (oculusViaOpenVR()) {
        qDebug() << "Oculus active via OpenVR";
    }

    _openVrDisplayActive = true;
    _container->setIsOptionChecked(StandingHMDSensorMode, true);

    _system->GetRecommendedRenderTargetSize(&_renderTargetSize.x, &_renderTargetSize.y);
    // Recommended render target size is per-eye, so double the X size for 
    // left + right eyes
    _renderTargetSize.x *= 2;

    withNonPresentThreadLock([&] {
        openvr_for_each_eye([&](vr::Hmd_Eye eye) {
            _eyeOffsets[eye] = toGlm(_system->GetEyeToHeadTransform(eye));
            _eyeProjections[eye] = toGlm(_system->GetProjectionMatrix(eye, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP));
        });
        // FIXME Calculate the proper combined projection by using GetProjectionRaw values from both eyes
        _cullingProjection = _eyeProjections[0];
    });

    // enable async time warp
    if (forceInterleavedReprojection) {
        vr::VRCompositor()->ForceInterleavedReprojectionOn(true);
    }

    // set up default sensor space such that the UI overlay will align with the front of the room.
    auto chaperone = vr::VRChaperone();
    if (chaperone) {
        float const UI_RADIUS = 1.0f;
        float const UI_HEIGHT = 1.6f;
        float const UI_Z_OFFSET = 0.5;

        float xSize, zSize;
        chaperone->GetPlayAreaSize(&xSize, &zSize);
        glm::vec3 uiPos(0.0f, UI_HEIGHT, UI_RADIUS - (0.5f * zSize) - UI_Z_OFFSET);
        _sensorResetMat = glm::inverse(createMatFromQuatAndPos(glm::quat(), uiPos));
    } else {
        #if DEV_BUILD
            qDebug() << "OpenVR: error could not get chaperone pointer";
        #endif
    }

    return Parent::internalActivate();
}

void OpenVrDisplayPlugin::internalDeactivate() {
    Parent::internalDeactivate();

    _openVrDisplayActive = false;
    _container->setIsOptionChecked(StandingHMDSensorMode, false);
    if (_system) {
        // TODO: Invalidate poses. It's fine if someone else sets these shared values, but we're about to stop updating them, and
        // we don't want ViveControllerManager to consider old values to be valid.
        _container->makeRenderingContextCurrent();
        releaseOpenVrSystem();
        _system = nullptr;
    }
}

void OpenVrDisplayPlugin::customizeContext() {
    // Display plugins in DLLs must initialize GL locally
    gl::initModuleGl();
    Parent::customizeContext();
}

void OpenVrDisplayPlugin::resetSensors() {
    glm::mat4 m;
    withNonPresentThreadLock([&] {
        m = toGlm(_nextSimPoseData.vrPoses[0].mDeviceToAbsoluteTracking);
    });
    _sensorResetMat = glm::inverse(cancelOutRollAndPitch(m));
}

static bool isBadPose(vr::HmdMatrix34_t* mat) {
    if (mat->m[1][3] < -0.2f) {
        return true;
    }
    return false;
}

bool OpenVrDisplayPlugin::beginFrameRender(uint32_t frameIndex) {
    PROFILE_RANGE_EX(render, __FUNCTION__, 0xff7fff00, frameIndex)
    handleOpenVrEvents();
    if (openVrQuitRequested()) {
        QMetaObject::invokeMethod(qApp, "quit");
        return false;
    }
    _currentRenderFrameInfo = FrameInfo();

    PoseData nextSimPoseData;
    withNonPresentThreadLock([&] {
        nextSimPoseData = _nextSimPoseData;
    });

    // HACK: when interface is launched and steam vr is NOT running, openvr will return bad HMD poses for a few frames
    // To workaround this, filter out any hmd poses that are obviously bad, i.e. beneath the floor.
    if (isBadPose(&nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking)) {
        // qDebug() << "WARNING: ignoring bad hmd pose from openvr";

        // use the last known good HMD pose
        nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking = _lastGoodHMDPose;
    } else {
        _lastGoodHMDPose = nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
    }

    vr::TrackedDeviceIndex_t handIndices[2] { vr::k_unTrackedDeviceIndexInvalid, vr::k_unTrackedDeviceIndexInvalid };
    {
        vr::TrackedDeviceIndex_t controllerIndices[2] ;
        auto trackedCount = _system->GetSortedTrackedDeviceIndicesOfClass(vr::TrackedDeviceClass_Controller, controllerIndices, 2);
        // Find the left and right hand controllers, if they exist
        for (uint32_t i = 0; i < std::min<uint32_t>(trackedCount, 2); ++i) {
            if (nextSimPoseData.vrPoses[i].bPoseIsValid) {
                auto role = _system->GetControllerRoleForTrackedDeviceIndex(controllerIndices[i]);
                if (vr::TrackedControllerRole_LeftHand == role) {
                    handIndices[0] = controllerIndices[i];
                } else if (vr::TrackedControllerRole_RightHand == role) {
                    handIndices[1] = controllerIndices[i];
                }
            }
        }
    }

    _currentRenderFrameInfo.renderPose = nextSimPoseData.poses[vr::k_unTrackedDeviceIndex_Hmd];
    bool keyboardVisible = isOpenVrKeyboardShown();

    std::array<mat4, 2> handPoses;
    if (!keyboardVisible) {
        for (int i = 0; i < 2; ++i) {
            if (handIndices[i] == vr::k_unTrackedDeviceIndexInvalid) {
                continue;
            }
            auto deviceIndex = handIndices[i];
            const mat4& mat = nextSimPoseData.poses[deviceIndex];
            const vec3& linearVelocity = nextSimPoseData.linearVelocities[deviceIndex];
            const vec3& angularVelocity = nextSimPoseData.angularVelocities[deviceIndex];
            auto correctedPose = openVrControllerPoseToHandPose(i == 0, mat, linearVelocity, angularVelocity);
            static const glm::quat HAND_TO_LASER_ROTATION = glm::rotation(Vectors::UNIT_Z, Vectors::UNIT_NEG_Y);
            handPoses[i] = glm::translate(glm::mat4(), correctedPose.translation) * glm::mat4_cast(correctedPose.rotation * HAND_TO_LASER_ROTATION);
        }
    }

    withNonPresentThreadLock([&] {
        _frameInfos[frameIndex] = _currentRenderFrameInfo;
    });
    return Parent::beginFrameRender(frameIndex);
}

void OpenVrDisplayPlugin::hmdPresent() {
    PROFILE_RANGE_EX(render, __FUNCTION__, 0xff00ff00, (uint64_t)_currentFrame->frameIndex)
    GLuint glTexId = getGLBackend()->getTextureID(_compositeFramebuffer->getRenderBuffer(0));
    vr::Texture_t vrTexture { (void*)(uintptr_t)glTexId, vr::TextureType_OpenGL, vr::ColorSpace_Auto };
    vr::VRCompositor()->Submit(vr::Eye_Left, &vrTexture, &OPENVR_TEXTURE_BOUNDS_LEFT);
    vr::VRCompositor()->Submit(vr::Eye_Right, &vrTexture, &OPENVR_TEXTURE_BOUNDS_RIGHT);
    vr::VRCompositor()->PostPresentHandoff();
    _presentRate.increment();

    vr::Compositor_FrameTiming newFrameTiming;
    memset(&newFrameTiming, 0, sizeof(vr::Compositor_FrameTiming));
    newFrameTiming.m_nSize = sizeof(vr::Compositor_FrameTiming);
    vr::VRCompositor()->GetFrameTiming(&newFrameTiming);
    _stutterRate.increment(newFrameTiming.m_nNumDroppedFrames);

    withPresentThreadLock([&] {
        _frameTiming = newFrameTiming;
    });
}

void OpenVrDisplayPlugin::postPreview() {
    PROFILE_RANGE_EX(render, __FUNCTION__, 0xff00ff00, (uint64_t)_currentFrame->frameIndex)
    PoseData nextRender, nextSim;
    nextRender.frameIndex = presentCount();

    _hmdActivityLevel = _system->GetTrackedDeviceActivityLevel(vr::k_unTrackedDeviceIndex_Hmd);

    vr::VRCompositor()->WaitGetPoses(nextRender.vrPoses, vr::k_unMaxTrackedDeviceCount, nextSim.vrPoses, vr::k_unMaxTrackedDeviceCount);

    glm::mat4 resetMat;
    withPresentThreadLock([&] {
        resetMat = _sensorResetMat;
    });
    nextRender.update(resetMat);
    nextSim.update(resetMat);
    withPresentThreadLock([&] {
        _nextSimPoseData = nextSim;
    });
    _nextRenderPoseData = nextRender;
}

bool OpenVrDisplayPlugin::isHmdMounted() const {
    return _hmdActivityLevel == vr::k_EDeviceActivityLevel_UserInteraction;
}

void OpenVrDisplayPlugin::updatePresentPose() {
    _currentPresentFrameInfo.presentPose = _nextRenderPoseData.poses[vr::k_unTrackedDeviceIndex_Hmd];
}

bool OpenVrDisplayPlugin::suppressKeyboard() { 
    if (isOpenVrKeyboardShown()) {
        return false;
    }
    if (!_keyboardSupressionCount.fetch_add(1)) {
        disableOpenVrKeyboard();
    }
    return true;
}

void OpenVrDisplayPlugin::unsuppressKeyboard() {
    if (_keyboardSupressionCount == 0) {
        qWarning() << "Attempted to unsuppress a keyboard that was not suppressed";
        return;
    }
    if (1 == _keyboardSupressionCount.fetch_sub(1)) {
        enableOpenVrKeyboard(_container);
    }
}

bool OpenVrDisplayPlugin::isKeyboardVisible() {
    return isOpenVrKeyboardShown(); 
}

QString OpenVrDisplayPlugin::getPreferredAudioInDevice() const {
    QString device = getVrSettingString(vr::k_pch_audio_Section, vr::k_pch_audio_OnPlaybackDevice_String);
    if (!device.isEmpty()) {
        static const WCHAR INIT = 0;
        size_t size = device.size() + 1;
        std::vector<WCHAR> deviceW;
        deviceW.assign(size, INIT);
        device.toWCharArray(deviceW.data());
        device = AudioClient::getWinDeviceName(deviceW.data());
    }
    return device;
}

QString OpenVrDisplayPlugin::getPreferredAudioOutDevice() const {
    QString device = getVrSettingString(vr::k_pch_audio_Section, vr::k_pch_audio_OnRecordDevice_String);
    if (!device.isEmpty()) {
        static const WCHAR INIT = 0;
        size_t size = device.size() + 1;
        std::vector<WCHAR> deviceW;
        deviceW.assign(size, INIT);
        device.toWCharArray(deviceW.data());
        device = AudioClient::getWinDeviceName(deviceW.data());
    }
    return device;
}

