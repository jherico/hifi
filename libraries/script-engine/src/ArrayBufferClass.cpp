//
//  ArrayBufferClass.cpp
//
//
//  Created by Clement on 7/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>

#include "ArrayBufferPrototype.h"
#include "DataViewClass.h"
#include "ScriptEngine.h"
#include "TypedArrays.h"

#include "ArrayBufferClass.h"

static const QString CLASS_NAME = "ArrayBuffer";

Q_DECLARE_METATYPE(QByteArray*)

// FIXME JSENGINE
#if 0
ArrayBufferClass::ArrayBufferClass(ScriptEngine* scriptEngine) :
QObject(scriptEngine),
QScriptClass(scriptEngine),
_scriptEngine(scriptEngine) {
    qScriptRegisterMetaType<QByteArray>(engine(), toScriptValue, fromScriptValue);
    QJSValue global = engine()->globalObject();
    
    // Save string handles for quick lookup
    _name = engine()->toStringHandle(CLASS_NAME.toLatin1());
    _byteLength = engine()->toStringHandle(BYTE_LENGTH_PROPERTY_NAME.toLatin1());
    
    // build prototype
    _proto = engine()->newQObject(new ArrayBufferPrototype(this),
                                QJSEngine::QtOwnership,
                                QJSEngine::SkipMethodsInEnumeration |
                                QJSEngine::ExcludeSuperClassMethods |
                                QJSEngine::ExcludeSuperClassProperties);
    _proto.setPrototype(global.property("Object").property("prototype"));
    
    // Register constructor
    _ctor = engine()->newFunction(construct, _proto);
    _ctor.setData(engine()->toScriptValue(this));
    
    engine()->globalObject().setProperty(name(), _ctor);
    
    // Registering other array types
    // The script engine is there parent so it'll delete them with itself
    new DataViewClass(scriptEngine);
    new Int8ArrayClass(scriptEngine);
    new Uint8ArrayClass(scriptEngine);
    new Uint8ClampedArrayClass(scriptEngine);
    new Int16ArrayClass(scriptEngine);
    new Uint16ArrayClass(scriptEngine);
    new Int32ArrayClass(scriptEngine);
    new Uint32ArrayClass(scriptEngine);
    new Float32ArrayClass(scriptEngine);
    new Float64ArrayClass(scriptEngine);
}

QJSValue ArrayBufferClass::newInstance(qint32 size) {
    const qint32 MAX_LENGTH = 100000000;
    if (size < 0) {
        engine()->evaluate("throw \"ArgumentError: negative length\"");
        return QJSValue();
    }
    if (size > MAX_LENGTH) {
        engine()->evaluate("throw \"ArgumentError: absurd length\"");
        return QJSValue();
    }
    
    engine()->reportAdditionalMemoryCost(size);
    QJSEngine* eng = engine();
    QVariant variant = QVariant::fromValue(QByteArray(size, 0));
    QJSValue data =  eng->newVariant(variant);
    return engine()->newObject(this, data);
}

QJSValue ArrayBufferClass::newInstance(const QByteArray& ba) {
    QJSValue data = engine()->newVariant(QVariant::fromValue(ba));
    return engine()->newObject(this, data);
}

QJSValue ArrayBufferClass::construct(QScriptContext* context, QJSEngine* engine) {
    ArrayBufferClass* cls = qscriptvalue_cast<ArrayBufferClass*>(context->callee().data());
    if (!cls) {
        // return if callee (function called) is not of type ArrayBuffer
        return QJSValue();
    }
    QJSValue arg = context->argument(0);
    if (!arg.isValid() || !arg.isNumber()) {
        return QJSValue();
    }
    
    quint32 size = arg.toInt32();
    QJSValue newObject = cls->newInstance(size);
    
    if (context->isCalledAsConstructor()) {
        // if called with keyword new, replace this object.
        context->setThisObject(newObject);
        return engine->undefinedValue();
    }
    
    return newObject;
}

QScriptClass::QueryFlags ArrayBufferClass::queryProperty(const QJSValue& object,
                                                    const QScriptString& name,
                                                    QueryFlags flags, uint* id) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data());
    if (ba && name == _byteLength) {
        // if the property queried is byteLength, only handle read access
        return flags &= HandlesReadAccess;
    }
    return 0; // No access
}

QJSValue ArrayBufferClass::property(const QJSValue& object,
                                   const QScriptString& name, uint id) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data());
    if (ba && name == _byteLength) {
        return ba->length();
    }
    return QJSValue();
}

QJSValue::PropertyFlags ArrayBufferClass::propertyFlags(const QJSValue& object,
                                                       const QScriptString& name, uint id) {
    return QJSValue::Undeletable;
}

QString ArrayBufferClass::name() const {
    return _name.toString();
}

QJSValue ArrayBufferClass::prototype() const {
    return _proto;
}

QJSValue ArrayBufferClass::toScriptValue(QJSEngine* engine, const QByteArray& ba) {
    QJSValue ctor = engine->globalObject().property(CLASS_NAME);
    ArrayBufferClass* cls = qscriptvalue_cast<ArrayBufferClass*>(ctor.data());
    if (!cls) {
        return engine->newVariant(QVariant::fromValue(ba));
    }
    return cls->newInstance(ba);
}

void ArrayBufferClass::fromScriptValue(const QJSValue& obj, QByteArray& ba) {
    ba = qvariant_cast<QByteArray>(obj.data().toVariant());
}

#endif
