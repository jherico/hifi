//  SpacemouseManager.h
//  interface/src/devices
//
//  Created by Marcel Verhagen on 09-06-15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SpacemouseManager_h
#define hifi_SpacemouseManager_h

#include <QtCore/QObject>
#include <controllers/UserInputMapper.h>
#include <controllers/InputDevice.h>
#include <controllers/StandardControls.h>

#include <plugins/InputPlugin.h>

#include <QtCore/QAbstractNativeEventFilter>

#include <Windows.h>

class SpacemouseManager : public InputPlugin, public QAbstractNativeEventFilter {
    Q_OBJECT
public:
    bool isSupported() const override;
    const QString getName() const override { return NAME; }
    const QString getID() const override { return NAME; }

    bool activate() override;
    void deactivate() override;

    void pluginFocusOutEvent() override;
    void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;
    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;

private:
    bool translateRawInputData(RAWINPUT& rawInput);

    ivec3 _translation;
    ivec3 _rotation;

    static const char* NAME;
    friend class SpacemouseDevice;
};

// connnects to the userinputmapper
class SpacemouseDevice : public controller::InputDevice {
public:
    SpacemouseDevice();
    enum PositionChannel {
        TRANSLATE_X,
        TRANSLATE_Y,
        TRANSLATE_Z,
        ROTATE_X,
        ROTATE_Y,
        ROTATE_Z,
    };

    enum ButtonChannel {
        BUTTON_1 = 1,
        BUTTON_2 = 2,
        BUTTON_3 = 3
    };

    typedef std::unordered_set<int> ButtonPressedMap;
    typedef std::map<int, float> AxisStateMap;

    float getButton(int channel) const;
    float getAxis(int channel) const;

    controller::Input makeInput(SpacemouseDevice::PositionChannel axis) const;
    controller::Input makeInput(SpacemouseDevice::ButtonChannel button) const;

    controller::Input::NamedPair makePair(SpacemouseDevice::PositionChannel axis, const QString& name) const;
    controller::Input::NamedPair makePair(SpacemouseDevice::ButtonChannel button, const QString& name) const;

    virtual controller::Input::NamedVector getAvailableInputs() const override;
    virtual QString getDefaultMappingConfig() const override;
    virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;
    virtual void focusOutEvent() override;

    glm::vec3 cc_position;
    glm::vec3 cc_rotation;
    int clientId;

    void setButton(int lastButtonState);
    void handleAxisEvent();
};

#endif // defined(hifi_SpacemouseManager_h)
