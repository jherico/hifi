//
//  3dRudderManager.cpp
//  input-plugins/src/input-plugins
//
//  Created by Nicolas PERRET 17/04/2018
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "3dRudderManager.h"

#include <QtCore/QCoreApplication>

#include <controllers/UserInputMapper.h>
#include <PerfStat.h>
#include <Preferences.h>
#include <SettingHandle.h>

const char* C3dRudderManager::NAME = "3dRudder";
const char* C3dRudderManager::_3DRUDDER_ID_STRING = "3dRudder";

const bool DEFAULT_ENABLED = false;

void CEvent::OnConnect(uint32_t nDeviceNumber)
{
	char sName[] = "3dRudder[0]";
	sName[9] = nDeviceNumber + 0x30;
	emit _pPlugin->RudderAdded(nDeviceNumber);
	emit _pPlugin->subdeviceConnected(_pPlugin->getName(), sName);

    _pPlugin->_3dRDevice = std::make_shared<C3dRudderInputDevice>(nDeviceNumber, _pPlugin->_pSdk);
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->registerDevice(_pPlugin->_3dRDevice);

}
void CEvent::OnDisconnect(uint32_t nDeviceNumber)
{
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->removeDevice(nDeviceNumber);

	emit _pPlugin->RudderRemoved(nDeviceNumber);
}


void C3dRudderManager::init() 
{
	loadSettings();
	auto preferences = DependencyManager::get<Preferences>();
	static const QString _3DRUDDER_PLUGIN{ "3dRudder Controller" };
	{
		auto getter = [this]()->bool { return _isEnabled; };
		auto setter = [this](bool value)
		{
			_isEnabled = value;
			saveSettings();
		};
		auto preference = new CheckPreference(_3DRUDDER_PLUGIN, "Enabled", getter, setter);
		preferences->addPreference(preference);
	}

	m_pEvent = new CEvent(this);

	_pSdk = ns3dRudder::GetSDK();
	_pSdk->Init();
	_pSdk->SetEvent(m_pEvent);
}

QStringList C3dRudderManager::getSubdeviceNames() 
{
	delete m_pEvent;
    return _subdeviceNames;
}

void C3dRudderManager::deinit() 
{
	saveSettings();
	ns3dRudder::EndSDK();
}

bool C3dRudderManager::activate() 
{
    loadSettings();
        		
	_subdeviceNames << "3dRudder";
	_isInitialized = true;

    InputPlugin::activate();

    return true;
}

void C3dRudderManager::deactivate() {
    InputPlugin::deactivate();
}

const char* SETTINGS_ENABLED_KEY = "enabled";

void C3dRudderManager::saveSettings() const {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        settings.setValue(QString(SETTINGS_ENABLED_KEY), _isEnabled);
    }
    settings.endGroup();
}

void C3dRudderManager::loadSettings() {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        _isEnabled = settings.value(SETTINGS_ENABLED_KEY, QVariant(DEFAULT_ENABLED)).toBool();
    }
    settings.endGroup();
}

bool C3dRudderManager::isSupported() const {
    return true;
}

void C3dRudderManager::pluginFocusOutEvent() {
    _3dRDevice->focusOutEvent();
}

void C3dRudderManager::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    if (!_isEnabled) 
	{
        return;
    }

    if (_isInitialized) 
	{
        _3dRDevice->update(deltaTime, inputCalibrationData);      
    }
}
