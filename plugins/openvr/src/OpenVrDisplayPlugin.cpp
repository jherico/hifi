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

PoseData _nextRenderPoseData;
PoseData _nextSimPoseData;

#define MIN_CORES_FOR_NORMAL_RENDER 5
bool forceInterleavedReprojection = (QThread::idealThreadCount() < MIN_CORES_FOR_NORMAL_RENDER);

static std::array<vr::Hmd_Eye, 2> VR_EYES{ { vr::Eye_Left, vr::Eye_Right } };
bool _openVrDisplayActive{ false };
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
    if (forceInterleavedReprojection && !_asyncReprojectionActive) {
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

const QString OpenVrDisplayPlugin::getName() const {
    std::string headsetName = getOpenVrDeviceName();
    if (headsetName == "HTC") {
        headsetName += " Vive";
    }

    return QString::fromStdString(headsetName);
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

    vr::Compositor_FrameTiming timing;
    memset(&timing, 0, sizeof(timing));
    timing.m_nSize = sizeof(vr::Compositor_FrameTiming);
    vr::VRCompositor()->GetFrameTiming(&timing);
    auto usingOpenVRForOculus = oculusViaOpenVR();
    _asyncReprojectionActive = (timing.m_nReprojectionFlags & VRCompositor_ReprojectionAsync) || usingOpenVRForOculus;

    if (usingOpenVRForOculus) {
        qDebug() << "Oculus active via OpenVR:  " << usingOpenVRForOculus;
    }
    qDebug() << "OpenVR Async Reprojection active:  " << _asyncReprojectionActive;

    _openVrDisplayActive = true;
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
        float const UI_HEIGHT = 0.0f;
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
    withNonPresentThreadLock([&] { m = toGlm(_nextSimPoseData.vrPoses[0].mDeviceToAbsoluteTracking); });
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
    withNonPresentThreadLock([&] { nextSimPoseData = _nextSimPoseData; });

    // HACK: when interface is launched and steam vr is NOT running, openvr will return bad HMD poses for a few frames
    // To workaround this, filter out any hmd poses that are obviously bad, i.e. beneath the floor.
    if (isBadPose(&nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking)) {
        // qDebug() << "WARNING: ignoring bad hmd pose from openvr";

        // use the last known good HMD pose
        nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking = _lastGoodHMDPose;
    } else {
        _lastGoodHMDPose = nextSimPoseData.vrPoses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
    }

    vr::TrackedDeviceIndex_t handIndices[2]{ vr::k_unTrackedDeviceIndexInvalid, vr::k_unTrackedDeviceIndexInvalid };
    {
        vr::TrackedDeviceIndex_t controllerIndices[2];
        auto trackedCount =
            _system->GetSortedTrackedDeviceIndicesOfClass(vr::TrackedDeviceClass_Controller, controllerIndices, 2);
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
            handPoses[i] = glm::translate(glm::mat4(), correctedPose.translation) *
                           glm::mat4_cast(correctedPose.rotation * HAND_TO_LASER_ROTATION);
        }
    }

    withNonPresentThreadLock([&] { _frameInfos[frameIndex] = _currentRenderFrameInfo; });
    return Parent::beginFrameRender(frameIndex);
}

void OpenVrDisplayPlugin::hmdPresent() {
    PROFILE_RANGE_EX(render, __FUNCTION__, 0xff00ff00, (uint64_t)_currentFrame->frameIndex)

    GLuint glTexId = getGLBackend()->getTextureID(_compositeFramebuffer->getRenderBuffer(0));
    vr::VRTextureWithPose_t vrTexture;
    vrTexture.handle = (void*)(uintptr_t)glTexId;
    vrTexture.eType = vr::TextureType_OpenGL;
    vrTexture.eColorSpace = vr::ColorSpace_Auto;
    vrTexture.mDeviceToAbsoluteTracking = toOpenVr(_currentRenderFrameInfo.renderPose);
    vr::VRCompositor()->Submit(vr::Eye_Left, &vrTexture, &OPENVR_TEXTURE_BOUNDS_LEFT, vr::Submit_TextureWithPose);
    vr::VRCompositor()->Submit(vr::Eye_Right, &vrTexture, &OPENVR_TEXTURE_BOUNDS_RIGHT, vr::Submit_TextureWithPose);
    vr::VRCompositor()->PostPresentHandoff();
    _presentRate.increment();

    vr::Compositor_FrameTiming frameTiming;
    memset(&frameTiming, 0, sizeof(vr::Compositor_FrameTiming));
    frameTiming.m_nSize = sizeof(vr::Compositor_FrameTiming);
    vr::VRCompositor()->GetFrameTiming(&frameTiming);
    _stutterRate.increment(frameTiming.m_nNumDroppedFrames);
}

void OpenVrDisplayPlugin::postPreview() {
    PROFILE_RANGE_EX(render, __FUNCTION__, 0xff00ff00, (uint64_t)_currentFrame->frameIndex)
    PoseData nextRender, nextSim;
    nextRender.frameIndex = presentCount();

    vr::VRCompositor()->WaitGetPoses(nextRender.vrPoses, vr::k_unMaxTrackedDeviceCount, nextSim.vrPoses,
                                        vr::k_unMaxTrackedDeviceCount);

    glm::mat4 resetMat;
    withPresentThreadLock([&] { resetMat = _sensorResetMat; });
    nextRender.update(resetMat);
    nextSim.update(resetMat);
    withPresentThreadLock([&] { _nextSimPoseData = nextSim; });

    if (isHmdMounted() != _hmdMounted) {
        _hmdMounted = !_hmdMounted;
        emit hmdMountedChanged();
    }
}

bool OpenVrDisplayPlugin::isHmdMounted() const {
    return isHeadInHeadset();
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

QRectF OpenVrDisplayPlugin::getPlayAreaRect() {
    auto chaperone = vr::VRChaperone();
    if (!chaperone) {
        qWarning() << "No chaperone";
        return QRectF();
    }

    if (chaperone->GetCalibrationState() >= vr::ChaperoneCalibrationState_Error) {
        qWarning() << "Chaperone status =" << chaperone->GetCalibrationState();
        return QRectF();
    }

    vr::HmdQuad_t rect;
    if (!chaperone->GetPlayAreaRect(&rect)) {
        qWarning() << "Chaperone rect not obtained";
        return QRectF();
    }

    auto minXZ = transformPoint(_sensorResetMat, toGlm(rect.vCorners[0]));
    auto maxXZ = minXZ;
    for (int i = 1; i < 4; i++) {
        auto point = transformPoint(_sensorResetMat, toGlm(rect.vCorners[i]));
        minXZ.x = std::min(minXZ.x, point.x);
        minXZ.z = std::min(minXZ.z, point.z);
        maxXZ.x = std::max(maxXZ.x, point.x);
        maxXZ.z = std::max(maxXZ.z, point.z);
    }

    glm::vec2 center = glm::vec2((minXZ.x + maxXZ.x) / 2, (minXZ.z + maxXZ.z) / 2);
    glm::vec2 dimensions = glm::vec2(maxXZ.x - minXZ.x, maxXZ.z - minXZ.z);

    return QRectF(center.x, center.y, dimensions.x, dimensions.y);
}

DisplayPlugin::StencilMaskMeshOperator OpenVrDisplayPlugin::getStencilMaskMeshOperator() {
    if (_system) {
        if (!_stencilMeshesInitialized) {
            _stencilMeshesInitialized = true;
            for (auto eye : VR_EYES) {
                vr::HiddenAreaMesh_t stencilMesh = _system->GetHiddenAreaMesh(eye);
                if (stencilMesh.pVertexData && stencilMesh.unTriangleCount > 0) {
                    std::vector<glm::vec3> vertices;
                    std::vector<uint32_t> indices;

                    const int NUM_INDICES_PER_TRIANGLE = 3;
                    int numIndices = stencilMesh.unTriangleCount * NUM_INDICES_PER_TRIANGLE;
                    vertices.reserve(numIndices);
                    indices.reserve(numIndices);
                    for (int i = 0; i < numIndices; i++) {
                        vr::HmdVector2_t vertex2D = stencilMesh.pVertexData[i];
                        // We need the vertices in clip space
                        vertices.emplace_back(vertex2D.v[0] - (1.0f - (float)eye), 2.0f * vertex2D.v[1] - 1.0f, 0.0f);
                        indices.push_back(i);
                    }

                    _stencilMeshes[eye] = graphics::Mesh::createIndexedTriangles_P3F((uint32_t)vertices.size(), (uint32_t)indices.size(), vertices.data(), indices.data());
                } else {
                    _stencilMeshesInitialized = false;
                }
            }
        }

        if (_stencilMeshesInitialized) {
            return [&](gpu::Batch& batch) {
                for (auto& mesh : _stencilMeshes) {
                    batch.setIndexBuffer(mesh->getIndexBuffer());
                    batch.setInputFormat((mesh->getVertexFormat()));
                    batch.setInputStream(0, mesh->getVertexStream());

                    // Draw
                    auto part = mesh->getPartBuffer().get<graphics::Mesh::Part>(0);
                    batch.drawIndexed(gpu::TRIANGLES, part._numIndices, part._startIndex);
                }
            };
        }
    }
    return nullptr;
}