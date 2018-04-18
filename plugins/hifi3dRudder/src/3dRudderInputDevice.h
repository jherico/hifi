//
//  3dRudderInputDevice.h
//  input-plugins/src/input-plugins
//
//  Created by Nicolas PERRET 17/04/2018
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_3dRudderInputDevice_h
#define hifi_3dRudderInputDevice_h

#include <qobject.h>
#include <qvector.h>

#include "3drudderSDK.h"


#include <controllers/InputDevice.h>
#include <controllers/StandardControls.h>

class C3dRudderInputDevice : public QObject, public controller::InputDevice {
    Q_OBJECT
    Q_PROPERTY(QString name READ getName)
    Q_PROPERTY(int instanceId READ getInstanceId)
    
public:
    using Pointer = std::shared_ptr<C3dRudderInputDevice>;

    const QString getName() const { return _name; }

    // Device functions
    virtual controller::Input::NamedVector getAvailableInputs() const override;
    virtual QString getDefaultMappingConfig() const override;
    virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;
    virtual void focusOutEvent() override;

    bool triggerHapticPulse(float strength, float duration, controller::Hand hand) override;
    
    C3dRudderInputDevice() : InputDevice("3dRudder") {}
    ~C3dRudderInputDevice();
    
    C3dRudderInputDevice(uint32_t nDeviceNumber, ns3dRudder::CSdk *pSdk);
    
    void closeC3dRudderInputDevice();
       
private:
    ns3dRudder::CSdk *_pSdk;
    uint32_t _nDeviceNumber;
    mutable controller::Input::NamedVector _availableInputs;
};

#endif // hifi_C3dRudderInputDevice_h
