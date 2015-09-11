//
//  AudioInjectorOptions.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <RegisteredMetaTypes.h>

#include "AudioInjectorOptions.h"

AudioInjectorOptions::AudioInjectorOptions() :
    position(0.0f, 0.0f, 0.0f),
    volume(1.0f),
    loop(false),
    orientation(glm::vec3(0.0f, 0.0f, 0.0f)),
    stereo(false),
    ignorePenumbra(false),
    localOnly(false),
    secondOffset(0.0)
{

}

QJSValue injectorOptionsToScriptValue(QJSEngine* engine, const AudioInjectorOptions& injectorOptions) {
    QJSValue obj = engine->newObject();
    obj.setProperty("position", vec3toScriptValue(engine, injectorOptions.position));
    obj.setProperty("volume", injectorOptions.volume);
    obj.setProperty("loop", injectorOptions.loop);
    obj.setProperty("orientation", quatToScriptValue(engine, injectorOptions.orientation));
    obj.setProperty("ignorePenumbra", injectorOptions.ignorePenumbra);
    obj.setProperty("localOnly", injectorOptions.localOnly);
    obj.setProperty("secondOffset", injectorOptions.secondOffset);
    return obj;
}

void injectorOptionsFromScriptValue(const QJSValue& object, AudioInjectorOptions& injectorOptions) {
    if (!object.property("position").isUndefined()) {
        vec3FromScriptValue(object.property("position"), injectorOptions.position);
    }
    
    if (!object.property("volume").isUndefined()) {
        injectorOptions.volume = object.property("volume").toNumber();
    }
    
    if (!object.property("loop").isUndefined()) {
        injectorOptions.loop = object.property("loop").toBool();
    }
    
    if (!object.property("orientation").isUndefined()) {
        quatFromScriptValue(object.property("orientation"), injectorOptions.orientation);
    }
    
    if (!object.property("ignorePenumbra").isUndefined()) {
        injectorOptions.ignorePenumbra = object.property("ignorePenumbra").toBool();
    }
    
    if (!object.property("localOnly").isUndefined()) {
        injectorOptions.localOnly = object.property("localOnly").toBool();
    }
    
    if (!object.property("secondOffset").isUndefined()) {
        injectorOptions.secondOffset = object.property("secondOffset").toNumber();
    }
 }