//
//  SpacemouseManager.cpp
//  interface/src/devices
//
//  Created by MarcelEdward Verhagen on 09-06-15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SpacemouseManager.h"

#include <array>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include <PathUtils.h>

const char* SpacemouseManager::NAME { "Spacemouse" };

#define LOGITECH_VENDOR_ID 0x46d

namespace spacemouse {
    bool isAttached() {
        unsigned int nDevices = 0;
        if (::GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0) {
            return false;
        }

        if (nDevices == 0) {
            return false;
        }

        std::vector<RAWINPUTDEVICELIST> rawInputDeviceList(nDevices);
        if (::GetRawInputDeviceList(&rawInputDeviceList[0], &nDevices, sizeof(RAWINPUTDEVICELIST)) == static_cast<unsigned int>(-1)) {
            return false;
        }

        RAWINPUTDEVICE deviceToRegister = { 0x01, 0x08, 0, 0 };

        for (unsigned int i = 0; i < nDevices; ++i) {
            RID_DEVICE_INFO rdi = { sizeof(rdi) };
            unsigned int cbSize = sizeof(rdi);

            if (::GetRawInputDeviceInfo(rawInputDeviceList[i].hDevice, RIDI_DEVICEINFO, &rdi, &cbSize) > 0) {
                //skip non HID and non logitec (3DConnexion) devices
                if (rdi.dwType != RIM_TYPEHID || rdi.hid.dwVendorId != LOGITECH_VENDOR_ID) {
                    continue;
                }

                //check if devices matches Multi-axis Controller
                if (deviceToRegister.usUsage == rdi.hid.usUsage && deviceToRegister.usUsagePage == rdi.hid.usUsagePage) {
                    return true;
                }
            }
        }
        return false;
    }


    enum Type {
        eSpacePilot = 0xc625,
        eSpaceNavigator = 0xc626,
        eSpaceExplorer = 0xc627,
        eSpaceNavigatorForNotebooks = 0xc628,
        eSpacePilotPRO = 0xc629
    };

    enum VirtualKey {
        V3DK_INVALID = 0,
        V3DK_MENU = 1, V3DK_FIT,
        V3DK_TOP, V3DK_LEFT, V3DK_RIGHT, V3DK_FRONT, V3DK_BOTTOM, V3DK_BACK,
        V3DK_CW, V3DK_CCW,
        V3DK_ISO1, V3DK_ISO2,
        V3DK_1, V3DK_2, V3DK_3, V3DK_4, V3DK_5, V3DK_6, V3DK_7, V3DK_8, V3DK_9, V3DK_10,
        V3DK_ESC, V3DK_ALT, V3DK_SHIFT, V3DK_CTRL,
        V3DK_ROTATE, V3DK_PANZOOM, V3DK_DOMINANT,
        V3DK_PLUS, V3DK_MINUS
    };

    struct VirtualKeys {
        Type pid;
        size_t nKeys;
        VirtualKey *vkeys;
    };

    static const VirtualKey SpaceExplorerKeys[] = {
        V3DK_INVALID,
        V3DK_1, V3DK_2,
        V3DK_TOP, V3DK_LEFT, V3DK_RIGHT, V3DK_FRONT,
        V3DK_ESC, V3DK_ALT, V3DK_SHIFT, V3DK_CTRL,
        V3DK_FIT, V3DK_MENU,
        V3DK_PLUS, V3DK_MINUS,
        V3DK_ROTATE
    };

    static const VirtualKey SpacePilotKeys[] = {
        V3DK_INVALID,
        V3DK_1, V3DK_2, V3DK_3, V3DK_4, V3DK_5, V3DK_6,
        V3DK_TOP, V3DK_LEFT, V3DK_RIGHT, V3DK_FRONT,
        V3DK_ESC, V3DK_ALT, V3DK_SHIFT, V3DK_CTRL,
        V3DK_FIT, V3DK_MENU,
        V3DK_PLUS, V3DK_MINUS,
        V3DK_DOMINANT, V3DK_ROTATE
    };

    static const struct VirtualKeys _3dmouseVirtualKeys[] =
    {
        { eSpacePilot, sizeof(SpacePilotKeys) / sizeof(SpacePilotKeys[0]), const_cast<VirtualKey *>(SpacePilotKeys) },
        { eSpaceExplorer, sizeof(SpaceExplorerKeys) / sizeof(SpaceExplorerKeys[0]), const_cast<VirtualKey *>(SpaceExplorerKeys) }
    };


    /*!
    Converts a hid device keycode (button identifier) of a pre-2009 3Dconnexion USB device to the standard 3d mouse virtual key definition.

    \a pid USB Product ID (PID) of 3D mouse device
    \a hidKeyCode  Hid keycode as retrieved from a Raw Input packet

    \return The standard 3d mouse virtual key (button identifier) or zero if an error occurs.

    Converts a hid device keycode (button identifier) of a pre-2009 3Dconnexion USB device
    to the standard 3d mouse virtual key definition.
    */

    VirtualKey hidKeyToVirtualKey(unsigned long pid, unsigned short hidKeyCode) {
        VirtualKey virtualkey = static_cast<VirtualKey>(hidKeyCode);
        for (size_t i = 0; i<sizeof(_3dmouseVirtualKeys) / sizeof(_3dmouseVirtualKeys[0]); ++i) {
            if (pid == _3dmouseVirtualKeys[i].pid)
            {
                if (hidKeyCode < _3dmouseVirtualKeys[i].nKeys)
                    virtualkey = _3dmouseVirtualKeys[i].vkeys[hidKeyCode];
                else
                    virtualkey = V3DK_INVALID;
                break;
            }
        }
        // Remaining devices are unchanged
        return virtualkey;
    }
}

#ifndef QWORD
#define QWORD uint64_t
#endif

#define LOGITECH_VENDOR_ID 0x46d
// object angular velocity per mouse tick 0.008 milliradians per second per count
#define ANGULAR_SCALE_RADIANS_PER_SECOND (8.0e-6f)
#define RAW_INPUT_BUFFER_SIZE 1024
#define HID_TRANSLATION_DATA 0x01
#define HID_ROTATION_DATA 0x02
#define HID_KEYPRESS_DATA 0x02



std::shared_ptr<SpacemouseDevice> instance;

bool SpacemouseManager::isSupported() const {
    return spacemouse::isAttached();
}

bool SpacemouseManager::activate() {
    // Usage Page = 0x01 Generic Desktop Page, Usage Id= 0x08 Multi-axis Controller
    RAWINPUTDEVICE deviceToRegister = { 0x01, 0x08, RIDEV_DEVNOTIFY, GetActiveWindow() };
    if (FALSE == ::RegisterRawInputDevices(&deviceToRegister, 1, sizeof(RAWINPUTDEVICE))) {
        qWarning() << "Failed to init";
        return false;
    }

    if (!instance) {
        instance = std::make_shared<SpacemouseDevice>();
    }

    if (instance->getDeviceID() == controller::Input::INVALID_DEVICE) {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        userInputMapper->registerDevice(instance);
    }

    qApp->installNativeEventFilter(this);
    InputPlugin::activate();
    return true;
}


void SpacemouseManager::deactivate() {
    qApp->removeNativeEventFilter(this);
    int deviceid = instance->getDeviceID();
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    userInputMapper->removeDevice(deviceid);
    InputPlugin::deactivate();
}


bool SpacemouseManager::nativeEventFilter(const QByteArray &eventType, void *message, long *result) {
    Q_UNUSED(eventType);
    MSG* msg = reinterpret_cast<MSG*>(message);
    if (msg->message == WM_INPUT) {
        auto hRawInput = reinterpret_cast<HRAWINPUT>(msg->lParam);
        static std::array<uint8_t, RAW_INPUT_BUFFER_SIZE> inputBuffer;
        PRAWINPUT rawInput = reinterpret_cast<PRAWINPUT>(inputBuffer.data());
        UINT cbSize = RAW_INPUT_BUFFER_SIZE;

        auto readSize = ::GetRawInputData(hRawInput, RID_INPUT, rawInput, &cbSize, sizeof(RAWINPUTHEADER));
        if (readSize == static_cast<UINT>(-1)) {
            return false;
        }

        translateRawInputData(*rawInput);
        ::DefRawInputProc(&rawInput, 1, sizeof(RAWINPUTHEADER));

        if (result != 0) {
            result = 0;
        }
        return true;
    }
    return false;
}

bool SpacemouseManager::translateRawInputData(RAWINPUT& rawInput) {
    // We are not interested in keyboard or mouse data received via raw input
    if (rawInput.header.dwType != RIM_TYPEHID) {
        return false;
    }

    RID_DEVICE_INFO sRidDeviceInfo;
    sRidDeviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
    UINT cbSize = sizeof(RID_DEVICE_INFO);
    if (::GetRawInputDeviceInfo(rawInput.header.hDevice, RIDI_DEVICEINFO, &sRidDeviceInfo, &cbSize) != sizeof(RID_DEVICE_INFO)) {
        return false;
    }
    if (sRidDeviceInfo.hid.dwVendorId != LOGITECH_VENDOR_ID) {
        return false;
    }

    if (rawInput.data.hid.bRawData[0] == HID_TRANSLATION_DATA) {
        short* pnRawData = reinterpret_cast<short*>(&rawInput.data.hid.bRawData[1]);
        _translation = { pnRawData[0], pnRawData[1], pnRawData[2] };
        if (rawInput.data.hid.dwSizeHid >= 13) {
            _rotation = { pnRawData[3], pnRawData[4], pnRawData[5] };
        }
        return true;
    } else if (rawInput.data.hid.bRawData[0] == HID_ROTATION_DATA) {
        short* pnRawData = reinterpret_cast<short*>(&rawInput.data.hid.bRawData[1]);
        _rotation = { pnRawData[0], pnRawData[1], pnRawData[2] };
        return true;
    } else if (rawInput.data.hid.bRawData[0] == HID_KEYPRESS_DATA) { // Keystate change
                                                                     // this is a package that contains 3d mouse keystate information
                                                                     // bit0=key1, bit=key2 etc.
        uint32_t dwKeystate = *reinterpret_cast<uint32_t*>(&rawInput.data.hid.bRawData[1]);
        // FIXME handle button states
        Q_UNUSED(dwKeystate);
        return true;
    }

    return false;
}

void SpacemouseManager::pluginFocusOutEvent() { 
    instance->focusOutEvent(); 
}

void SpacemouseManager::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    const vec3 SCALE(350);
    instance->cc_position = glm::clamp(vec3(_translation) / SCALE, -1.0f, 1.0f);
    instance->cc_rotation = glm::clamp(vec3(_rotation) / SCALE, -1.0f, 1.0f); 
    instance->handleAxisEvent();
    _translation = _rotation = ivec3();
}

SpacemouseDevice::SpacemouseDevice() : InputDevice(SpacemouseManager::NAME) { }

void SpacemouseDevice::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};


void SpacemouseDevice::handleAxisEvent() {
    const auto& rotation = cc_rotation;
    _axisStateMap[ROTATE_X] = rotation.x;
    _axisStateMap[ROTATE_Y] = rotation.y;
    _axisStateMap[ROTATE_Z] = rotation.z;
    const auto& position = cc_position;
    _axisStateMap[TRANSLATE_X] = position.x;
    _axisStateMap[TRANSLATE_Y] = position.y;
    _axisStateMap[TRANSLATE_Z] = position.z;
}

void SpacemouseDevice::setButton(int lastButtonState) {
    _buttonPressedMap.clear();
    _buttonPressedMap.insert(lastButtonState);
}


controller::Input::NamedVector SpacemouseDevice::getAvailableInputs() const {
    using namespace controller;

    static const Input::NamedVector availableInputs{
        makePair(BUTTON_1, "LeftButton"),
        makePair(BUTTON_2, "RightButton"),
        makePair(TRANSLATE_X, "TranslateX"),
        makePair(TRANSLATE_Y, "TranslateY"),
        makePair(TRANSLATE_Z, "TranslateZ"),
        makePair(ROTATE_X, "RotateX"),
        makePair(ROTATE_Y, "RotateY"),
        makePair(ROTATE_Z, "RotateZ"),
    };
    return availableInputs;
}

QString SpacemouseDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/spacemouse.json";
    return MAPPING_JSON;
}

float SpacemouseDevice::getButton(int channel) const {
    if (!_buttonPressedMap.empty()) {
        if (_buttonPressedMap.find(channel) != _buttonPressedMap.end()) {
            return 1.0f;
        } else {
            return 0.0f;
        }
    }
    return 0.0f;
}

float SpacemouseDevice::getAxis(int channel) const {
    auto axis = _axisStateMap.find(channel);
    if (axis != _axisStateMap.end()) {
        return (*axis).second;
    } else {
        return 0.0f;
    }
}

controller::Input SpacemouseDevice::makeInput(SpacemouseDevice::ButtonChannel button) const {
    return controller::Input(_deviceID, button, controller::ChannelType::BUTTON);
}

controller::Input SpacemouseDevice::makeInput(SpacemouseDevice::PositionChannel axis) const {
    return controller::Input(_deviceID, axis, controller::ChannelType::AXIS);
}

controller::Input::NamedPair SpacemouseDevice::makePair(SpacemouseDevice::ButtonChannel button, const QString& name) const {
    return controller::Input::NamedPair(makeInput(button), name);
}

controller::Input::NamedPair SpacemouseDevice::makePair(SpacemouseDevice::PositionChannel axis, const QString& name) const {
    return controller::Input::NamedPair(makeInput(axis), name);
}

void SpacemouseDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    // the update is done in the SpacemouseManager class.
    // for windows in the nativeEventFilter the inputmapper is connected or registed or removed when an 3Dconnnexion device is attached or detached
    // for osx the api will call DeviceAddedHandler or DeviceRemoveHandler when a 3Dconnexion device is attached or detached
}

