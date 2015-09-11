//
//  ArrayBufferViewClass.h
//
//
//  Created by Clement on 7/8/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ArrayBufferViewClass_h
#define hifi_ArrayBufferViewClass_h

#include <QtCore/QObject>
#include <QtQml/QJSEngine>

#include "ScriptEngine.h"

#if 0
static const QString BUFFER_PROPERTY_NAME = "buffer";
static const QString BYTE_OFFSET_PROPERTY_NAME = "byteOffset";
static const QString BYTE_LENGTH_PROPERTY_NAME = "byteLength";

class ArrayBufferViewClass : public QObject, public QScriptClass {
    Q_OBJECT
public:
    ArrayBufferViewClass(ScriptEngine* scriptEngine);
    
    ScriptEngine* getScriptEngine() { return _scriptEngine; }
    
    virtual QueryFlags queryProperty(const QJSValue& object,
                                     const QScriptString& name,
                                     QueryFlags flags, uint* id);
    virtual QJSValue property(const QJSValue& object,
                                  const QScriptString& name, uint id);
    virtual QJSValue::PropertyFlags propertyFlags(const QJSValue& object,
                                                      const QScriptString& name, uint id);
protected:
    // JS Object attributes
    QScriptString _bufferName;
    QScriptString _byteOffsetName;
    QScriptString _byteLengthName;
    
    ScriptEngine* _scriptEngine;
};

#endif
#endif // hifi_ArrayBufferViewClass_h