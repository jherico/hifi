//
//  AudioEffectOptions.cpp
//  libraries/audio/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioEffectOptions.h"

static const QString MAX_ROOM_SIZE_HANDLE = "maxRoomSize";
static const QString ROOM_SIZE_HANDLE = "roomSize";
static const QString REVERB_TIME_HANDLE = "reverbTime";
static const QString DAMPIMG_HANDLE = "damping";
static const QString SPREAD_HANDLE = "spread";
static const QString INPUT_BANDWIDTH_HANDLE = "inputBandwidth";
static const QString EARLY_LEVEL_HANDLE = "earlyLevel";
static const QString TAIL_LEVEL_HANDLE = "tailLevel";
static const QString DRY_LEVEL_HANDLE = "dryLevel";
static const QString WET_LEVEL_HANDLE = "wetLevel";

AudioEffectOptions::AudioEffectOptions(QJSValue arguments) :
    _maxRoomSize(50.0f),
    _roomSize(50.0f),
    _reverbTime(4.0f),
    _damping(0.5f),
    _spread(15.0f),
    _inputBandwidth(0.75f),
    _earlyLevel(-22.0f),
    _tailLevel(-28.0f),
    _dryLevel(0.0f),
    _wetLevel(6.0f) {
        if (arguments.property(MAX_ROOM_SIZE_HANDLE).isNumber()) {
            _maxRoomSize = arguments.property(MAX_ROOM_SIZE_HANDLE).toNumber();
        }
        if (arguments.property(ROOM_SIZE_HANDLE).isNumber()) {
            _roomSize = arguments.property(ROOM_SIZE_HANDLE).toNumber();
        }
        if (arguments.property(REVERB_TIME_HANDLE).isNumber()) {
            _reverbTime = arguments.property(REVERB_TIME_HANDLE).toNumber();
        }
        if (arguments.property(DAMPIMG_HANDLE).isNumber()) {
            _damping = arguments.property(DAMPIMG_HANDLE).toNumber();
        }
        if (arguments.property(SPREAD_HANDLE).isNumber()) {
            _spread = arguments.property(SPREAD_HANDLE).toNumber();
        }
        if (arguments.property(INPUT_BANDWIDTH_HANDLE).isNumber()) {
            _inputBandwidth = arguments.property(INPUT_BANDWIDTH_HANDLE).toNumber();
        }
        if (arguments.property(EARLY_LEVEL_HANDLE).isNumber()) {
            _earlyLevel = arguments.property(EARLY_LEVEL_HANDLE).toNumber();
        }
        if (arguments.property(TAIL_LEVEL_HANDLE).isNumber()) {
            _tailLevel = arguments.property(TAIL_LEVEL_HANDLE).toNumber();
        }
        if (arguments.property(DRY_LEVEL_HANDLE).isNumber()) {
            _dryLevel = arguments.property(DRY_LEVEL_HANDLE).toNumber();
        }
        if (arguments.property(WET_LEVEL_HANDLE).isNumber()) {
            _wetLevel = arguments.property(WET_LEVEL_HANDLE).toNumber();
        }
}

AudioEffectOptions::AudioEffectOptions(const AudioEffectOptions &other) : QObject() {
    *this = other;
}

AudioEffectOptions& AudioEffectOptions::operator=(const AudioEffectOptions &other) {
    _maxRoomSize = other._maxRoomSize;
    _roomSize = other._roomSize;
    _reverbTime = other._reverbTime;
    _damping = other._damping;
    _spread = other._spread;
    _inputBandwidth = other._inputBandwidth;
    _earlyLevel = other._earlyLevel;
    _tailLevel = other._tailLevel;
    _dryLevel = other._dryLevel;
    _wetLevel = other._wetLevel;
    
    return *this;
}

