//
//  RegisteredMetaTypes.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 10/3/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QUrl>
#include <QtCore/QUuid>
#include <QtCore/QRect>
#include <QtGui/QColor>
#include <QtQml/QJSEngine>

#include <glm/gtc/quaternion.hpp>

#include "RegisteredMetaTypes.h"

static int vec4MetaTypeId = qRegisterMetaType<glm::vec4>();
static int vec3MetaTypeId = qRegisterMetaType<glm::vec3>();
static int qVectorVec3MetaTypeId = qRegisterMetaType<QVector<glm::vec3>>();
static int vec2MetaTypeId = qRegisterMetaType<glm::vec2>();
static int quatMetaTypeId = qRegisterMetaType<glm::quat>();
static int xColorMetaTypeId = qRegisterMetaType<xColor>();
static int pickRayMetaTypeId = qRegisterMetaType<PickRay>();
static int collisionMetaTypeId = qRegisterMetaType<Collision>();
static int qMapURLStringMetaTypeId = qRegisterMetaType<QMap<QUrl,QString>>();

void registerMetaTypes(QJSEngine* engine) {
    //qScriptRegisterMetaType(engine, vec4toScriptValue, vec4FromScriptValue);
    //qScriptRegisterMetaType(engine, vec3toScriptValue, vec3FromScriptValue);
    //qScriptRegisterMetaType(engine, qVectorVec3ToScriptValue, qVectorVec3FromScriptValue);
    //qScriptRegisterMetaType(engine, qVectorFloatToScriptValue, qVectorFloatFromScriptValue);
    //qScriptRegisterMetaType(engine, vec2toScriptValue, vec2FromScriptValue);
    //qScriptRegisterMetaType(engine, quatToScriptValue, quatFromScriptValue);
    //qScriptRegisterMetaType(engine, qRectToScriptValue, qRectFromScriptValue);
    //qScriptRegisterMetaType(engine, xColorToScriptValue, xColorFromScriptValue);
    //qScriptRegisterMetaType(engine, qColorToScriptValue, qColorFromScriptValue);
    //qScriptRegisterMetaType(engine, qURLToScriptValue, qURLFromScriptValue);
    //qScriptRegisterMetaType(engine, pickRayToScriptValue, pickRayFromScriptValue);
    //qScriptRegisterMetaType(engine, collisionToScriptValue, collisionFromScriptValue);
    //qScriptRegisterMetaType(engine, quuidToScriptValue, quuidFromScriptValue);
    //qScriptRegisterMetaType(engine, qSizeFToScriptValue, qSizeFFromScriptValue);
}

QJSValue vec4toScriptValue(QJSEngine* engine, const glm::vec4& vec4) {
    QJSValue obj = engine->newObject();
    obj.setProperty("x", vec4.x);
    obj.setProperty("y", vec4.y);
    obj.setProperty("z", vec4.z);
    obj.setProperty("w", vec4.w);
    return obj;
}

void vec4FromScriptValue(const QJSValue& object, glm::vec4& vec4) {
    vec4.x = object.property("x").toVariant().toFloat();
    vec4.y = object.property("y").toVariant().toFloat();
    vec4.z = object.property("z").toVariant().toFloat();
    vec4.w = object.property("w").toVariant().toFloat();
}

QJSValue vec3toScriptValue(QJSEngine* engine, const glm::vec3 &vec3) {
    QJSValue obj = engine->newObject();
    if (vec3.x != vec3.x || vec3.y != vec3.y || vec3.z != vec3.z) {
        // if vec3 contains a NaN don't try to convert it
        return obj;
    }
    obj.setProperty("x", vec3.x);
    obj.setProperty("y", vec3.y);
    obj.setProperty("z", vec3.z);
    return obj;
}

void vec3FromScriptValue(const QJSValue &object, glm::vec3 &vec3) {
    vec3.x = object.property("x").toVariant().toFloat();
    vec3.y = object.property("y").toVariant().toFloat();
    vec3.z = object.property("z").toVariant().toFloat();
}

QJSValue qVectorVec3ToScriptValue(QJSEngine* engine, const QVector<glm::vec3>& vector) {
    QJSValue array = engine->newArray();
    for (int i = 0; i < vector.size(); i++) {
        array.setProperty(i, vec3toScriptValue(engine, vector.at(i)));
    }
    return array;
}

QVector<float> qVectorFloatFromScriptValue(const QJSValue& array) {
    if(!array.isArray()) {
        return QVector<float>();
    }
    QVector<float> newVector;
    int length = array.property("length").toInt();
    newVector.reserve(length);
    for (int i = 0; i < length; i++) {
        if(array.property(i).isNumber()) {
            newVector << array.property(i).toNumber();
        }
    }
    
    return newVector;
}

QJSValue qVectorFloatToScriptValue(QJSEngine* engine, const QVector<float>& vector) {
    QJSValue array = engine->newArray();
    for (int i = 0; i < vector.size(); i++) {
        float num = vector.at(i);
        array.setProperty(i, QJSValue(num));
    }
    return array;
}

void qVectorFloatFromScriptValue(const QJSValue& array, QVector<float>& vector) {
    int length = array.property("length").toInt();
    
    for (int i = 0; i < length; i++) {
        vector << array.property(i).toVariant().toFloat();
    }
}
//
QVector<glm::vec3> qVectorVec3FromScriptValue(const QJSValue& array){
    QVector<glm::vec3> newVector;
    int length = array.property("length").toInt();
    
    for (int i = 0; i < length; i++) {
        glm::vec3 newVec3 = glm::vec3();
        vec3FromScriptValue(array.property(i), newVec3);
        newVector << newVec3;
    }
    return newVector;
}

void qVectorVec3FromScriptValue(const QJSValue& array, QVector<glm::vec3>& vector ) {
    int length = array.property("length").toInt();
    
    for (int i = 0; i < length; i++) {
        glm::vec3 newVec3 = glm::vec3();
        vec3FromScriptValue(array.property(i), newVec3);
        vector << newVec3;
    }
}

QJSValue vec2toScriptValue(QJSEngine* engine, const glm::vec2 &vec2) {
    QJSValue obj = engine->newObject();
    obj.setProperty("x", vec2.x);
    obj.setProperty("y", vec2.y);
    return obj;
}

void vec2FromScriptValue(const QJSValue &object, glm::vec2 &vec2) {
    vec2.x = object.property("x").toVariant().toFloat();
    vec2.y = object.property("y").toVariant().toFloat();
}

QJSValue quatToScriptValue(QJSEngine* engine, const glm::quat& quat) {
    QJSValue obj = engine->newObject();
    obj.setProperty("x", quat.x);
    obj.setProperty("y", quat.y);
    obj.setProperty("z", quat.z);
    obj.setProperty("w", quat.w);
    return obj;
}

void quatFromScriptValue(const QJSValue &object, glm::quat& quat) {
    quat.x = object.property("x").toVariant().toFloat();
    quat.y = object.property("y").toVariant().toFloat();
    quat.z = object.property("z").toVariant().toFloat();
    quat.w = object.property("w").toVariant().toFloat();
}

QJSValue qRectToScriptValue(QJSEngine* engine, const QRect& rect) {
    QJSValue obj = engine->newObject();
    obj.setProperty("x", rect.x());
    obj.setProperty("y", rect.y());
    obj.setProperty("width", rect.width());
    obj.setProperty("height", rect.height());
    return obj;
}

void qRectFromScriptValue(const QJSValue &object, QRect& rect) {
    rect.setX(object.property("x").toVariant().toInt());
    rect.setY(object.property("y").toVariant().toInt());
    rect.setWidth(object.property("width").toVariant().toInt());
    rect.setHeight(object.property("height").toVariant().toInt());
}

QJSValue xColorToScriptValue(QJSEngine *engine, const xColor& color) {
    QJSValue obj = engine->newObject();
    obj.setProperty("red", color.red);
    obj.setProperty("green", color.green);
    obj.setProperty("blue", color.blue);
    return obj;
}

void xColorFromScriptValue(const QJSValue &object, xColor& color) {
    if (object.isUndefined() || object.isNull()) {
        return;
    }
    if (object.isNumber()) {
        color.red = color.green = color.blue = (uint8_t)object.toInt();
    } else if (object.isString()) {
        QColor qcolor(object.toString());
        if (qcolor.isValid()) {
            color.red = (uint8_t)qcolor.red();
            color.blue = (uint8_t)qcolor.blue();
            color.green = (uint8_t)qcolor.green();
        }
    } else {
        color.red = object.property("red").toVariant().toInt();
        color.green = object.property("green").toVariant().toInt();
        color.blue = object.property("blue").toVariant().toInt();
    }
}

QJSValue qColorToScriptValue(QJSEngine* engine, const QColor& color) {
    QJSValue object = engine->newObject();
    object.setProperty("red", color.red());
    object.setProperty("green", color.green());
    object.setProperty("blue", color.blue());
    object.setProperty("alpha", color.alpha());
    return object;
}

void qColorFromScriptValue(const QJSValue& object, QColor& color) {
    if (object.isNumber()) {
        color.setRgb(object.toInt());
    
    } else if (object.isString()) {
        color.setNamedColor(object.toString());
            
    } else {
        QJSValue alphaValue = object.property("alpha");
        color.setRgb(object.property("red").toInt(), object.property("green").toInt(), object.property("blue").toInt(),
            alphaValue.isNumber() ? alphaValue.toInt() : 255);
    }
}

QJSValue qURLToScriptValue(QJSEngine* engine, const QUrl& url) {
    return url.toString();
}

void qURLFromScriptValue(const QJSValue& object, QUrl& url) {
    url = object.toString();
}

QJSValue pickRayToScriptValue(QJSEngine* engine, const PickRay& pickRay) {
    QJSValue obj = engine->newObject();
    QJSValue origin = vec3toScriptValue(engine, pickRay.origin);
    obj.setProperty("origin", origin);
    QJSValue direction = vec3toScriptValue(engine, pickRay.direction);
    obj.setProperty("direction", direction);
    return obj;
}

void pickRayFromScriptValue(const QJSValue& object, PickRay& pickRay) {
    QJSValue originValue = object.property("origin");
    if (originValue.isObject()) {
        auto x = originValue.property("x");
        auto y = originValue.property("y");
        auto z = originValue.property("z");
        if (x.isNumber() && y.isNumber() && z.isNumber()) {
            pickRay.origin.x = x.toNumber();
            pickRay.origin.y = y.toNumber();
            pickRay.origin.z = z.toNumber();
        }
    }
    QJSValue directionValue = object.property("direction");
    if (directionValue.isObject()) {
        auto x = directionValue.property("x");
        auto y = directionValue.property("y");
        auto z = directionValue.property("z");
        if (x.isNumber() && y.isNumber() && z.isNumber()) {
            pickRay.direction.x = x.toNumber();
            pickRay.direction.y = y.toNumber();
            pickRay.direction.z = z.toNumber();
        }
    }
}

QJSValue collisionToScriptValue(QJSEngine* engine, const Collision& collision) {
    QJSValue obj = engine->newObject();
    obj.setProperty("type", collision.type);
    obj.setProperty("idA", quuidToScriptValue(engine, collision.idA));
    obj.setProperty("idB", quuidToScriptValue(engine, collision.idB));
    obj.setProperty("penetration", vec3toScriptValue(engine, collision.penetration));
    obj.setProperty("contactPoint", vec3toScriptValue(engine, collision.contactPoint));
    obj.setProperty("velocityChange", vec3toScriptValue(engine, collision.velocityChange));
    return obj;
}

void collisionFromScriptValue(const QJSValue &object, Collision& collision) {
    // TODO: implement this when we know what it means to accept collision events from JS
}

QJSValue quuidToScriptValue(QJSEngine* engine, const QUuid& uuid) {
    if (uuid.isNull()) {
        return QJSValue::NullValue;
    }
    QJSValue obj(uuid.toString());
    return obj;
}

void quuidFromScriptValue(const QJSValue& object, QUuid& uuid) {
    if (object.isNull()) {
        uuid = QUuid();
        return;
    }
    QString uuidAsString = object.toVariant().toString();
    QUuid fromString(uuidAsString);
    uuid = fromString;
}

QJSValue qSizeFToScriptValue(QJSEngine* engine, const QSizeF& qSizeF) {
    QJSValue obj = engine->newObject();
    obj.setProperty("width", qSizeF.width());
    obj.setProperty("height", qSizeF.height());
    return obj;
}

void qSizeFFromScriptValue(const QJSValue& object, QSizeF& qSizeF) {
    qSizeF.setWidth(object.property("width").toVariant().toFloat());
    qSizeF.setHeight(object.property("height").toVariant().toFloat());
}
