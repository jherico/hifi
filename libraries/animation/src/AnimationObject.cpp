//
//  AnimationObject.cpp
//  libraries/animation/src/
//
//  Created by Andrzej Kapolka on 4/17/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtQml/QJSEngine>

#include "AnimationCache.h"
#include "AnimationObject.h"
// FIXME JSENGINE
#if 0

QStringList AnimationObject::getJointNames() const {
    return qscriptvalue_cast<AnimationPointer>(thisObject())->getJointNames();
}

QVector<FBXAnimationFrame> AnimationObject::getFrames() const {
    return qscriptvalue_cast<AnimationPointer>(thisObject())->getFrames();
}

QVector<glm::quat> AnimationFrameObject::getRotations() const {
    return qscriptvalue_cast<FBXAnimationFrame>(thisObject()).rotations;
}

#endif

void registerAnimationTypes(QJSEngine* engine) {
    // FIXME JSENGINE
#if 0
    qScriptRegisterSequenceMetaType<QVector<FBXAnimationFrame> >(engine);
    engine->setDefaultPrototype(qMetaTypeId<FBXAnimationFrame>(), engine->newQObject(
        new AnimationFrameObject(), QJSEngine::ScriptOwnership));
    engine->setDefaultPrototype(qMetaTypeId<AnimationPointer>(), engine->newQObject(
        new AnimationObject(), QJSEngine::ScriptOwnership));
#endif
}

