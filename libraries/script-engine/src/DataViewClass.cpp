//
//  DataViewClass.cpp
//
//
//  Created by Clement on 7/8/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DataViewPrototype.h"

#include "DataViewClass.h"

Q_DECLARE_METATYPE(QByteArray*)

static const QString DATA_VIEW_NAME = "DataView";

// FIXME JSENGINE
#if 0

DataViewClass::DataViewClass(ScriptEngine* scriptEngine) : ArrayBufferViewClass(scriptEngine) {
    QJSValue global = engine()->globalObject();
    
    // Save string handles for quick lookup
    _name = engine()->toStringHandle(DATA_VIEW_NAME.toLatin1());
    
    // build prototype
    _proto = engine()->newQObject(new DataViewPrototype(this),
                                QJSEngine::QtOwnership,
                                QJSEngine::SkipMethodsInEnumeration |
                                QJSEngine::ExcludeSuperClassMethods |
                                QJSEngine::ExcludeSuperClassProperties);
    _proto.setPrototype(global.property("Object").property("prototype"));
    
    // Register constructor
    _ctor = engine()->newFunction(construct, _proto);
    _ctor.setData(engine()->toScriptValue(this));
    engine()->globalObject().setProperty(name(), _ctor);
}

QJSValue DataViewClass::newInstance(QJSValue buffer, quint32 byteOffset, quint32 byteLentgh) {
    QJSValue data = engine()->newObject();
    data.setProperty(_bufferName, buffer);
    data.setProperty(_byteOffsetName, byteOffset);
    data.setProperty(_byteLengthName, byteLentgh);
    
    return engine()->newObject(this, data);
}

QJSValue DataViewClass::construct(QScriptContext* context, QJSEngine* engine) {
    DataViewClass* cls = qscriptvalue_cast<DataViewClass*>(context->callee().data());
    if (!cls || context->argumentCount() < 1) {
        return QJSValue();
    }
    
    QJSValue bufferArg = context->argument(0);
    QJSValue byteOffsetArg = (context->argumentCount() >= 2) ? context->argument(1) : QJSValue();
    QJSValue byteLengthArg = (context->argumentCount() >= 3) ? context->argument(2) : QJSValue();
    
    QByteArray* arrayBuffer = (bufferArg.isValid()) ? qscriptvalue_cast<QByteArray*>(bufferArg.data()) :NULL;
    if (!arrayBuffer) {
        engine->evaluate("throw \"TypeError: 1st argument not a ArrayBuffer\"");
        return QJSValue();
    }
    if (byteOffsetArg.isNumber() &&
        (byteOffsetArg.toInt32() < 0 ||
         byteOffsetArg.toInt32() > arrayBuffer->size())) {
            engine->evaluate("throw \"RangeError: byteOffset out of range\"");
            return QJSValue();
        }
    if (byteLengthArg.isNumber() &&
        (byteLengthArg.toInt32() < 0 ||
         byteOffsetArg.toInt32() + byteLengthArg.toInt32() > arrayBuffer->size())) {
            engine->evaluate("throw \"RangeError: byteLength out of range\"");
            return QJSValue();
        }
    quint32 byteOffset = (byteOffsetArg.isNumber()) ? byteOffsetArg.toInt32() : 0;
    quint32 byteLength = (byteLengthArg.isNumber()) ? byteLengthArg.toInt32() : arrayBuffer->size() - byteOffset;
    QJSValue newObject = cls->newInstance(bufferArg, byteOffset, byteLength);
    
    if (context->isCalledAsConstructor()) {
        context->setThisObject(newObject);
        return engine->undefinedValue();
    }
    
    return newObject;
}

QString DataViewClass::name() const {
    return _name.toString();
}

QJSValue DataViewClass::prototype() const {
    return _proto;
}
#endif
