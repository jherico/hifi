//
//  AudioEffectOptions.h
//  libraries/audio/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioEffectOptions_h
#define hifi_AudioEffectOptions_h

#include <QObject>
#include <QtQml/QJSEngine>

class AudioEffectOptions : public QObject {
    Q_OBJECT

    // Meters Square
    Q_PROPERTY(float maxRoomSize READ getMaxRoomSize WRITE setMaxRoomSize)
    Q_PROPERTY(float roomSize READ getRoomSize WRITE setRoomSize)

    // Seconds
    Q_PROPERTY(float reverbTime READ getReverbTime WRITE setReverbTime)

    // Ratio between 0 and 1
    Q_PROPERTY(float damping READ getDamping WRITE setDamping)

    // (?) Does not appear to be set externally very often
    Q_PROPERTY(float spread READ getSpread WRITE setSpread)

    // Ratio between 0 and 1
    Q_PROPERTY(float inputBandwidth READ getInputBandwidth WRITE setInputBandwidth)

    // in dB
    Q_PROPERTY(float earlyLevel READ getEarlyLevel WRITE setEarlyLevel)
    Q_PROPERTY(float tailLevel READ getTailLevel WRITE setTailLevel)
    Q_PROPERTY(float dryLevel READ getDryLevel WRITE setDryLevel)
    Q_PROPERTY(float wetLevel READ getWetLevel WRITE setWetLevel)

public:
    AudioEffectOptions(QJSValue arguments = QJSValue());
    AudioEffectOptions(const AudioEffectOptions &other);
    AudioEffectOptions& operator=(const AudioEffectOptions &other);

    float getRoomSize() const { return _roomSize; }
    void setRoomSize(float roomSize ) { _roomSize = roomSize; }

    float getMaxRoomSize() const { return _maxRoomSize; }
    void setMaxRoomSize(float maxRoomSize ) { _maxRoomSize = maxRoomSize; }

    float getReverbTime() const { return _reverbTime; }
    void setReverbTime(float reverbTime ) { _reverbTime = reverbTime; }

    float getDamping() const { return _damping; }
    void setDamping(float damping ) { _damping = damping; }

    float getSpread() const { return _spread; }
    void setSpread(float spread ) { _spread = spread; }

    float getInputBandwidth() const { return _inputBandwidth; }
    void setInputBandwidth(float inputBandwidth ) { _inputBandwidth = inputBandwidth; }

    float getEarlyLevel() const { return _earlyLevel; }
    void setEarlyLevel(float earlyLevel ) { _earlyLevel = earlyLevel; }

    float getTailLevel() const { return _tailLevel; }
    void setTailLevel(float tailLevel ) { _tailLevel = tailLevel; }

    float getDryLevel() const { return _dryLevel; }
    void setDryLevel(float dryLevel) { _dryLevel = dryLevel; }

    float getWetLevel() const { return _wetLevel; }
    void setWetLevel(float wetLevel) { _wetLevel = wetLevel; }

private:
    // http://wiki.audacityteam.org/wiki/GVerb#Instant_Reverberb_settings

    // Meters Square
    float _maxRoomSize;
    float _roomSize;

    // Seconds
    float _reverbTime;

    // Ratio between 0 and 1
    float _damping;

    // ? (Does not appear to be set externally very often)
    float _spread;

    // Ratio between 0 and 1
    float _inputBandwidth;

    // dB
    float _earlyLevel;
    float _tailLevel;
    float _dryLevel;
    float _wetLevel;
};

#endif // hifi_AudioEffectOptions_h
