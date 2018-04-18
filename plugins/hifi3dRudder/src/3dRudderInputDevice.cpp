//
//  3dRudderInputDevice.cpp
//  input-plugins/src/input-plugins
//
//  Created by Nicolas PERRET 17/04/2018
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "3dRudderInputDevice.h"

#include <PathUtils.h>

C3dRudderInputDevice::C3dRudderInputDevice(uint32_t nDeviceNumber, ns3dRudder::CSdk *pSdk) :InputDevice("3dRudder")
{
}

C3dRudderInputDevice::~C3dRudderInputDevice() {
    closeC3dRudderInputDevice();
}

void C3dRudderInputDevice::closeC3dRudderInputDevice() {
}

void C3dRudderInputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {

	ns3dRudder::Axis lAxis;
	ns3dRudder::CurveArray  ACurve;
	if ( _pSdk->IsDeviceConnected(_nDeviceNumber) )
	{
		ns3dRudder::Status status = _pSdk->GetStatus(_nDeviceNumber);
		if ( status >= ns3dRudder::InUse )
		{
			if ( _pSdk->GetAxis(_nDeviceNumber, ns3dRudder::ValueWithCurveNonSymmetricalPitch, &lAxis, &ACurve) != ns3dRudder::NotReady )
			{
				_axisStateMap[makeInput(controller::StandardAxisChannel::LX).getChannel()] = lAxis.GetXAxis();
				_axisStateMap[makeInput(controller::StandardAxisChannel::LY).getChannel()] = lAxis.GetYAxis();
				_axisStateMap[makeInput(controller::StandardAxisChannel::RX).getChannel()] = lAxis.GetZRotation();
			}
		}
	}
}

void C3dRudderInputDevice::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};


bool C3dRudderInputDevice::triggerHapticPulse(float strength, float duration, controller::Hand hand) {
    
    return false;
}

controller::Input::NamedVector C3dRudderInputDevice::getAvailableInputs() const {
    using namespace controller;
    if (_availableInputs.length() == 0) {
        _availableInputs = 
		{
            makePair(LX, "aX"),
            makePair(LY, "aY"),
            makePair(RX, "rZ"),
            makePair(RY, "aZ"),
        };
    }
    return _availableInputs;
}

QString C3dRudderInputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/3drudder.json";
    return MAPPING_JSON;
}
