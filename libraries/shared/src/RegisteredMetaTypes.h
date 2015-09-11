//
//  RegisteredMetaTypes.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 10/3/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RegisteredMetaTypes_h
#define hifi_RegisteredMetaTypes_h

#include <QtQml/QJSEngine>
#include <QtCore/QUuid>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "SharedUtil.h"

class QColor;
class QUrl;

Q_DECLARE_METATYPE(glm::vec4)
Q_DECLARE_METATYPE(glm::vec3)
Q_DECLARE_METATYPE(glm::vec2)
Q_DECLARE_METATYPE(glm::quat)
Q_DECLARE_METATYPE(xColor)
Q_DECLARE_METATYPE(QVector<glm::vec3>)
Q_DECLARE_METATYPE(QVector<float>)

void registerMetaTypes(QJSEngine* engine);

QJSValue vec4toScriptValue(QJSEngine* engine, const glm::vec4& vec4);
void vec4FromScriptValue(const QJSValue& object, glm::vec4& vec4);

QJSValue vec3toScriptValue(QJSEngine* engine, const glm::vec3 &vec3);
void vec3FromScriptValue(const QJSValue &object, glm::vec3 &vec3);

QJSValue vec2toScriptValue(QJSEngine* engine, const glm::vec2 &vec2);
void vec2FromScriptValue(const QJSValue &object, glm::vec2 &vec2);

QJSValue quatToScriptValue(QJSEngine* engine, const glm::quat& quat);
void quatFromScriptValue(const QJSValue &object, glm::quat& quat);

QJSValue qRectToScriptValue(QJSEngine* engine, const QRect& rect);
void qRectFromScriptValue(const QJSValue& object, QRect& rect);

QJSValue xColorToScriptValue(QJSEngine* engine, const xColor& color);
void xColorFromScriptValue(const QJSValue &object, xColor& color);

QJSValue qColorToScriptValue(QJSEngine* engine, const QColor& color);
void qColorFromScriptValue(const QJSValue& object, QColor& color);

QJSValue qURLToScriptValue(QJSEngine* engine, const QUrl& url);
void qURLFromScriptValue(const QJSValue& object, QUrl& url);

QJSValue qVectorVec3ToScriptValue(QJSEngine* engine, const QVector<glm::vec3>& vector);
void qVectorVec3FromScriptValue(const QJSValue& array, QVector<glm::vec3>& vector);
QVector<glm::vec3> qVectorVec3FromScriptValue( const QJSValue& array);

QJSValue qVectorFloatToScriptValue(QJSEngine* engine, const QVector<float>& vector);
void qVectorFloatFromScriptValue(const QJSValue& array, QVector<float>& vector);
QVector<float> qVectorFloatFromScriptValue(const QJSValue& array);

class PickRay {
public:
    PickRay() : origin(0.0f), direction(0.0f)  { }
    PickRay(const glm::vec3& origin, const glm::vec3 direction) : origin(origin), direction(direction) {}
    glm::vec3 origin;
    glm::vec3 direction;
};
Q_DECLARE_METATYPE(PickRay)
QJSValue pickRayToScriptValue(QJSEngine* engine, const PickRay& pickRay);
void pickRayFromScriptValue(const QJSValue& object, PickRay& pickRay);

enum ContactEventType {
    CONTACT_EVENT_TYPE_START, 
    CONTACT_EVENT_TYPE_CONTINUE,
    CONTACT_EVENT_TYPE_END                                                                                            
};                                                                                                                    

class Collision {
public:
    Collision() : type(CONTACT_EVENT_TYPE_START), idA(), idB(), contactPoint(0.0f), penetration(0.0f), velocityChange(0.0f) { }
    Collision(ContactEventType cType, const QUuid& cIdA, const QUuid& cIdB, const glm::vec3& cPoint,
              const glm::vec3& cPenetration, const glm::vec3& velocityChange)
    :   type(cType), idA(cIdA), idB(cIdB), contactPoint(cPoint), penetration(cPenetration), velocityChange(velocityChange) { }

    ContactEventType type;
    QUuid idA;
    QUuid idB;
    glm::vec3 contactPoint;
    glm::vec3 penetration;
    glm::vec3 velocityChange;
};
Q_DECLARE_METATYPE(Collision)
QJSValue collisionToScriptValue(QJSEngine* engine, const Collision& collision);
void collisionFromScriptValue(const QJSValue &object, Collision& collision);

//Q_DECLARE_METATYPE(QUuid) // don't need to do this for QUuid since it's already a meta type
QJSValue quuidToScriptValue(QJSEngine* engine, const QUuid& uuid);
void quuidFromScriptValue(const QJSValue& object, QUuid& uuid);

//Q_DECLARE_METATYPE(QSizeF) // Don't need to to this becase it's arleady a meta type
QJSValue qSizeFToScriptValue(QJSEngine* engine, const QSizeF& qSizeF);
void qSizeFFromScriptValue(const QJSValue& object, QSizeF& qSizeF);

#endif // hifi_RegisteredMetaTypes_h
