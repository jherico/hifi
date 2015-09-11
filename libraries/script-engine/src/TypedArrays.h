//
//  TypedArrays.h
//
//
//  Created by Clement on 7/9/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TypedArrays_h
#define hifi_TypedArrays_h

#include "ArrayBufferViewClass.h"

#if 0

static const QString BYTES_PER_ELEMENT_PROPERTY_NAME = "BYTES_PER_ELEMENT";
static const QString LENGTH_PROPERTY_NAME = "length";

static const QString INT_8_ARRAY_CLASS_NAME = "Int8Array";
static const QString UINT_8_ARRAY_CLASS_NAME = "Uint8Array";
static const QString UINT_8_CLAMPED_ARRAY_CLASS_NAME = "Uint8ClampedArray";
static const QString INT_16_ARRAY_CLASS_NAME = "Int16Array";
static const QString UINT_16_ARRAY_CLASS_NAME = "Uint16Array";
static const QString INT_32_ARRAY_CLASS_NAME = "Int32Array";
static const QString UINT_32_ARRAY_CLASS_NAME = "Uint32Array";
static const QString FLOAT_32_ARRAY_CLASS_NAME = "Float32Array";
static const QString FLOAT_64_ARRAY_CLASS_NAME = "Float64Array";

class TypedArray : public ArrayBufferViewClass {
    Q_OBJECT
public:
    TypedArray(ScriptEngine* scriptEngine, QString name);
    virtual QJSValue newInstance(quint32 length);
    virtual QJSValue newInstance(QJSValue array);
    virtual QJSValue newInstance(QJSValue buffer, quint32 byteOffset, quint32 length);
    
    virtual QueryFlags queryProperty(const QJSValue& object,
                             const QScriptString& name,
                             QueryFlags flags, uint* id);
    virtual QJSValue property(const QJSValue& object,
                                  const QScriptString& name, uint id);
    virtual void setProperty(QJSValue& object, const QScriptString& name, uint id, const QJSValue& value) = 0;
    virtual QJSValue::PropertyFlags propertyFlags(const QJSValue& object,
                                                      const QScriptString& name, uint id);
    
    QString name() const;
    QJSValue prototype() const;
    
protected:
    static QJSValue construct(QScriptContext* context, QJSEngine* engine);
    
    void setBytesPerElement(quint32 bytesPerElement);
    
    QJSValue _proto;
    QJSValue _ctor;
    
    QScriptString _name;
    QScriptString _bytesPerElementName;
    QScriptString _lengthName;
    
    quint32 _bytesPerElement;
    
    friend class TypedArrayPrototype;
};

class Int8ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Int8ArrayClass(ScriptEngine* scriptEngine);
    
    QJSValue property(const QJSValue& object, const QScriptString& name, uint id);
    void setProperty(QJSValue& object, const QScriptString& name, uint id, const QJSValue& value);
};

class Uint8ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Uint8ArrayClass(ScriptEngine* scriptEngine);
    
    QJSValue property(const QJSValue& object, const QScriptString& name, uint id);
    void setProperty(QJSValue& object, const QScriptString& name, uint id, const QJSValue& value);
};

class Uint8ClampedArrayClass : public TypedArray {
    Q_OBJECT
public:
    Uint8ClampedArrayClass(ScriptEngine* scriptEngine);
    
    QJSValue property(const QJSValue& object, const QScriptString& name, uint id);
    void setProperty(QJSValue& object, const QScriptString& name, uint id, const QJSValue& value);
};

class Int16ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Int16ArrayClass(ScriptEngine* scriptEngine);
    
    QJSValue property(const QJSValue& object, const QScriptString& name, uint id);
    void setProperty(QJSValue& object, const QScriptString& name, uint id, const QJSValue& value);
};

class Uint16ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Uint16ArrayClass(ScriptEngine* scriptEngine);
    
    QJSValue property(const QJSValue& object, const QScriptString& name, uint id);
    void setProperty(QJSValue& object, const QScriptString& name, uint id, const QJSValue& value);
};

class Int32ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Int32ArrayClass(ScriptEngine* scriptEngine);
    
    QJSValue property(const QJSValue& object, const QScriptString& name, uint id);
    void setProperty(QJSValue& object, const QScriptString& name, uint id, const QJSValue& value);
};

class Uint32ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Uint32ArrayClass(ScriptEngine* scriptEngine);
    
    QJSValue property(const QJSValue& object, const QScriptString& name, uint id);
    void setProperty(QJSValue& object, const QScriptString& name, uint id, const QJSValue& value);
};

class Float32ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Float32ArrayClass(ScriptEngine* scriptEngine);
    
    QJSValue property(const QJSValue& object, const QScriptString& name, uint id);
    void setProperty(QJSValue& object, const QScriptString& name, uint id, const QJSValue& value);
};

class Float64ArrayClass : public TypedArray {
    Q_OBJECT
public:
    Float64ArrayClass(ScriptEngine* scriptEngine);
    
    QJSValue property(const QJSValue& object, const QScriptString& name, uint id);
    void setProperty(QJSValue& object, const QScriptString& name, uint id, const QJSValue& value);
};

#endif

#endif // hifi_TypedArrays_h