//
//  TypedArrays.cpp
//
//
//  Created by Clement on 7/9/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/glm.hpp>

#include "ScriptEngine.h"
#include "TypedArrayPrototype.h"

#include "TypedArrays.h"

Q_DECLARE_METATYPE(QByteArray*)
// FIXME JSENGINE
#if 0

TypedArray::TypedArray(ScriptEngine* scriptEngine, QString name) : ArrayBufferViewClass(scriptEngine) {
    _bytesPerElementName = engine()->toStringHandle(BYTES_PER_ELEMENT_PROPERTY_NAME.toLatin1());
    _lengthName = engine()->toStringHandle(LENGTH_PROPERTY_NAME.toLatin1());
    _name = engine()->toStringHandle(name.toLatin1());
    
    QJSValue global = engine()->globalObject();
    
    // build prototype
    _proto = engine()->newQObject(new TypedArrayPrototype(this),
                                  QJSEngine::QtOwnership,
                                  QJSEngine::SkipMethodsInEnumeration |
                                  QJSEngine::ExcludeSuperClassMethods |
                                  QJSEngine::ExcludeSuperClassProperties);
    _proto.setPrototype(global.property("Object").property("prototype"));
    
    // Register constructor
    _ctor = engine()->newFunction(construct, _proto);
    _ctor.setData(engine()->toScriptValue(this));
    engine()->globalObject().setProperty(_name, _ctor);
}

QJSValue TypedArray::newInstance(quint32 length) {
    ArrayBufferClass* array = getScriptEngine()->getArrayBufferClass();
    QJSValue buffer = array->newInstance(length * _bytesPerElement);
    return newInstance(buffer, 0, length);
}

QJSValue TypedArray::newInstance(QJSValue array) {
    const QString ARRAY_LENGTH_HANDLE = "length";
    if (array.property(ARRAY_LENGTH_HANDLE).isValid()) {
        quint32 length = array.property(ARRAY_LENGTH_HANDLE).toInt32();
        QJSValue newArray = newInstance(length);
        for (quint32 i = 0; i < length; ++i) {
            QJSValue value = array.property(QString::number(i));
            setProperty(newArray, engine()->toStringHandle(QString::number(i)),
                        i * _bytesPerElement, (value.isNumber()) ? value : QJSValue(0));
        }
        return newArray;
    }
    engine()->evaluate("throw \"ArgumentError: not an array\"");
    return QJSValue();
}

QJSValue TypedArray::newInstance(QJSValue buffer, quint32 byteOffset, quint32 length) {
    QJSValue data = engine()->newObject();
    data.setProperty(_bufferName, buffer);
    data.setProperty(_byteOffsetName, byteOffset);
    data.setProperty(_byteLengthName, length * _bytesPerElement);
    data.setProperty(_lengthName, length);
    
    return engine()->newObject(this, data);
}

QJSValue TypedArray::construct(QScriptContext* context, QJSEngine* engine) {
    TypedArray* cls = qscriptvalue_cast<TypedArray*>(context->callee().data());
    if (!cls) {
        return QJSValue();
    }
    if (context->argumentCount() == 0) {
        return cls->newInstance(0);
    }
    
    QJSValue newObject;
    QJSValue bufferArg = context->argument(0);
    QByteArray* arrayBuffer = qscriptvalue_cast<QByteArray*>(bufferArg.data());
    
    // parse arguments
    if (arrayBuffer) {
        if (context->argumentCount() == 1) {
            // Case for entire ArrayBuffer
            newObject = cls->newInstance(bufferArg, 0, arrayBuffer->size());
        } else {
            QJSValue byteOffsetArg = context->argument(1);
            if (!byteOffsetArg.isNumber()) {
                engine->evaluate("throw \"ArgumentError: 2nd arg is not a number\"");
                return QJSValue();
            }
            if (byteOffsetArg.toInt32() < 0 || byteOffsetArg.toInt32() > arrayBuffer->size()) {
                engine->evaluate("throw \"RangeError: byteOffset out of range\"");
                return QJSValue();
            }
            if (byteOffsetArg.toInt32() % cls->_bytesPerElement != 0) {
                engine->evaluate("throw \"RangeError: byteOffset not a multiple of BYTES_PER_ELEMENT\"");
            }
            quint32 byteOffset = byteOffsetArg.toInt32();
            
            if (context->argumentCount() == 2) {
                // case for end of ArrayBuffer
                if ((arrayBuffer->size() - byteOffset) % cls->_bytesPerElement != 0) {
                    engine->evaluate("throw \"RangeError: byteLength - byteOffset not a multiple of BYTES_PER_ELEMENT\"");
                }
                quint32 length = (arrayBuffer->size() - byteOffset) / cls->_bytesPerElement;
                newObject = cls->newInstance(bufferArg, byteOffset, length);
            } else {
                
                QJSValue lengthArg = (context->argumentCount() > 2) ? context->argument(2) : QJSValue();
                if (!lengthArg.isNumber()) {
                    engine->evaluate("throw \"ArgumentError: 3nd arg is not a number\"");
                    return QJSValue();
                }
                if (lengthArg.toInt32() < 0 ||
                    byteOffsetArg.toInt32() + lengthArg.toInt32() * (qint32)(cls->_bytesPerElement) > arrayBuffer->size()) {
                    engine->evaluate("throw \"RangeError: byteLength out of range\"");
                    return QJSValue();
                }
                quint32 length = lengthArg.toInt32();
                
                // case for well-defined range
                newObject = cls->newInstance(bufferArg, byteOffset, length);
            }
        }
    } else if (context->argument(0).isNumber()) {
        // case for new ArrayBuffer
        newObject = cls->newInstance(context->argument(0).toInt32());
    } else {
        newObject = cls->newInstance(bufferArg);
    }
    
    if (context->isCalledAsConstructor()) {
        // if called with the new keyword, replace this object
        context->setThisObject(newObject);
        return engine->undefinedValue();
    }
    
    return newObject;
}

QScriptClass::QueryFlags TypedArray::queryProperty(const QJSValue& object,
                                                   const QScriptString& name,
                                                   QueryFlags flags, uint* id) {
    if (name == _bytesPerElementName || name == _lengthName) {
        return flags &= HandlesReadAccess; // Only keep read access flags
    }
    
    quint32 byteOffset = object.data().property(_byteOffsetName).toInt32();
    quint32 length = object.data().property(_lengthName).toInt32();
    bool ok = false;
    quint32 pos = name.toArrayIndex(&ok);
    
    // Check that name is a valid index and arrayBuffer exists
    if (ok && pos < length) {
        *id = byteOffset + pos * _bytesPerElement; // save pos to avoid recomputation
        return HandlesReadAccess | HandlesWriteAccess; // Read/Write access
    }
    
    return ArrayBufferViewClass::queryProperty(object, name, flags, id);
}

QJSValue TypedArray::property(const QJSValue& object,
                                      const QScriptString& name, uint id) {
    if (name == _bytesPerElementName) {
        return QJSValue(_bytesPerElement);
    }
    if (name == _lengthName) {
        return object.data().property(_lengthName);
    }
    return ArrayBufferViewClass::property(object, name, id);
}

QJSValue::PropertyFlags TypedArray::propertyFlags(const QJSValue& object,
                                                      const QScriptString& name, uint id) {
    return QJSValue::Undeletable;
}

QString TypedArray::name() const {
    return _name.toString();
}

QJSValue TypedArray::prototype() const {
    return _proto;
}

void TypedArray::setBytesPerElement(quint32 bytesPerElement) {
    _bytesPerElement = bytesPerElement;
    _ctor.setProperty(_bytesPerElementName, _bytesPerElement);
}

// templated helper functions
// don't work for floats as they require single precision settings
template<class T>
QJSValue propertyHelper(const QByteArray* arrayBuffer, const QScriptString& name, uint id) {
    bool ok = false;
    name.toArrayIndex(&ok);
    
    if (ok && arrayBuffer) {
        QDataStream stream(*arrayBuffer);
        stream.skipRawData(id);
        
        T result;
        stream >> result;
        return result;
    }
    return QJSValue();
}

template<class T>
void setPropertyHelper(QByteArray* arrayBuffer, const QScriptString& name, uint id, const QJSValue& value) {
    if (arrayBuffer && value.isNumber()) {
        QDataStream stream(arrayBuffer, QIODevice::ReadWrite);
        stream.skipRawData(id);
        
        stream << (T)value.toNumber();
    }
}

Int8ArrayClass::Int8ArrayClass(ScriptEngine* scriptEngine) : TypedArray(scriptEngine, INT_8_ARRAY_CLASS_NAME) {
    setBytesPerElement(sizeof(qint8));
}

QJSValue Int8ArrayClass::property(const QJSValue& object, const QScriptString& name, uint id) {
    QByteArray* arrayBuffer = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    QJSValue result = propertyHelper<qint8>(arrayBuffer, name, id);
    return (result.isValid()) ? result : TypedArray::property(object, name, id);
}

void Int8ArrayClass::setProperty(QJSValue &object, const QScriptString &name,
                                 uint id, const QJSValue& value) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    setPropertyHelper<qint8>(ba, name, id, value);
}

Uint8ArrayClass::Uint8ArrayClass(ScriptEngine* scriptEngine) : TypedArray(scriptEngine, UINT_8_ARRAY_CLASS_NAME) {
    setBytesPerElement(sizeof(quint8));
}

QJSValue Uint8ArrayClass::property(const QJSValue& object, const QScriptString& name, uint id) {
    QByteArray* arrayBuffer = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    QJSValue result = propertyHelper<quint8>(arrayBuffer, name, id);
    return (result.isValid()) ? result : TypedArray::property(object, name, id);
}

void Uint8ArrayClass::setProperty(QJSValue& object, const QScriptString& name,
                                  uint id, const QJSValue& value) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    setPropertyHelper<quint8>(ba, name, id, value);
}

Uint8ClampedArrayClass::Uint8ClampedArrayClass(ScriptEngine* scriptEngine) : TypedArray(scriptEngine, UINT_8_CLAMPED_ARRAY_CLASS_NAME) {
    setBytesPerElement(sizeof(quint8));
}

QJSValue Uint8ClampedArrayClass::property(const QJSValue& object, const QScriptString& name, uint id) {
    QByteArray* arrayBuffer = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    QJSValue result = propertyHelper<quint8>(arrayBuffer, name, id);
    return (result.isValid()) ? result : TypedArray::property(object, name, id);
}

void Uint8ClampedArrayClass::setProperty(QJSValue& object, const QScriptString& name,
                                  uint id, const QJSValue& value) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    if (ba && value.isNumber()) {
        QDataStream stream(ba, QIODevice::ReadWrite);
        stream.skipRawData(id);
        if (value.toNumber() > 255) {
            stream << (quint8)255;
        } else if (value.toNumber() < 0) {
            stream << (quint8)0;
        } else {
            stream << (quint8)glm::clamp(qRound(value.toNumber()), 0, 255);
        }
    }
}

Int16ArrayClass::Int16ArrayClass(ScriptEngine* scriptEngine) : TypedArray(scriptEngine, INT_16_ARRAY_CLASS_NAME) {
    setBytesPerElement(sizeof(qint16));
}

QJSValue Int16ArrayClass::property(const QJSValue& object, const QScriptString& name, uint id) {
    QByteArray* arrayBuffer = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    QJSValue result = propertyHelper<qint16>(arrayBuffer, name, id);
    return (result.isValid()) ? result : TypedArray::property(object, name, id);
}

void Int16ArrayClass::setProperty(QJSValue& object, const QScriptString& name,
                                  uint id, const QJSValue& value) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    setPropertyHelper<qint16>(ba, name, id, value);
}

Uint16ArrayClass::Uint16ArrayClass(ScriptEngine* scriptEngine) : TypedArray(scriptEngine, UINT_16_ARRAY_CLASS_NAME) {
    setBytesPerElement(sizeof(quint16));
}

QJSValue Uint16ArrayClass::property(const QJSValue& object, const QScriptString& name, uint id) {
    QByteArray* arrayBuffer = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    QJSValue result = propertyHelper<quint16>(arrayBuffer, name, id);
    return (result.isValid()) ? result : TypedArray::property(object, name, id);
}

void Uint16ArrayClass::setProperty(QJSValue& object, const QScriptString& name,
                                  uint id, const QJSValue& value) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    setPropertyHelper<quint16>(ba, name, id, value);
}

Int32ArrayClass::Int32ArrayClass(ScriptEngine* scriptEngine) : TypedArray(scriptEngine, INT_32_ARRAY_CLASS_NAME) {
    setBytesPerElement(sizeof(qint32));
}

QJSValue Int32ArrayClass::property(const QJSValue& object, const QScriptString& name, uint id) {
    QByteArray* arrayBuffer = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    QJSValue result = propertyHelper<qint32>(arrayBuffer, name, id);
    return (result.isValid()) ? result : TypedArray::property(object, name, id);
}

void Int32ArrayClass::setProperty(QJSValue& object, const QScriptString& name,
                                  uint id, const QJSValue& value) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    setPropertyHelper<qint32>(ba, name, id, value);
}

Uint32ArrayClass::Uint32ArrayClass(ScriptEngine* scriptEngine) : TypedArray(scriptEngine, UINT_32_ARRAY_CLASS_NAME) {
    setBytesPerElement(sizeof(quint32));
}

QJSValue Uint32ArrayClass::property(const QJSValue& object, const QScriptString& name, uint id) {
    
    QByteArray* arrayBuffer = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    QJSValue result = propertyHelper<quint32>(arrayBuffer, name, id);
    return (result.isValid()) ? result : TypedArray::property(object, name, id);
}

void Uint32ArrayClass::setProperty(QJSValue& object, const QScriptString& name,
                                  uint id, const QJSValue& value) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    setPropertyHelper<quint32>(ba, name, id, value);
}

Float32ArrayClass::Float32ArrayClass(ScriptEngine* scriptEngine) : TypedArray(scriptEngine, FLOAT_32_ARRAY_CLASS_NAME) {
    setBytesPerElement(sizeof(float));
}

QJSValue Float32ArrayClass::property(const QJSValue& object, const QScriptString& name, uint id) {
    QByteArray* arrayBuffer = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());bool ok = false;
    name.toArrayIndex(&ok);
    
    if (ok && arrayBuffer) {
        QDataStream stream(*arrayBuffer);
        stream.skipRawData(id);
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        
        float result;
        stream >> result;
        if (isNaN(result)) {
            return QJSValue();
        }
        return result;
    }
    return TypedArray::property(object, name, id);
}

void Float32ArrayClass::setProperty(QJSValue& object, const QScriptString& name,
                                  uint id, const QJSValue& value) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    if (ba && value.isNumber()) {
        QDataStream stream(ba, QIODevice::ReadWrite);
        stream.skipRawData(id);
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        
        stream << (float)value.toNumber();
    }
}

Float64ArrayClass::Float64ArrayClass(ScriptEngine* scriptEngine) : TypedArray(scriptEngine, FLOAT_64_ARRAY_CLASS_NAME) {
    setBytesPerElement(sizeof(double));
}

QJSValue Float64ArrayClass::property(const QJSValue& object, const QScriptString& name, uint id) {
    QByteArray* arrayBuffer = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());bool ok = false;
    name.toArrayIndex(&ok);
    
    if (ok && arrayBuffer) {
        QDataStream stream(*arrayBuffer);
        stream.skipRawData(id);
        stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
        
        double result;
        stream >> result;
        if (isNaN(result)) {
            return QJSValue();
        }
        return result;
    }
    return TypedArray::property(object, name, id);
}

void Float64ArrayClass::setProperty(QJSValue& object, const QScriptString& name,
                                  uint id, const QJSValue& value) {
    QByteArray* ba = qscriptvalue_cast<QByteArray*>(object.data().property(_bufferName).data());
    if (ba && value.isNumber()) {
        QDataStream stream(ba, QIODevice::ReadWrite);
        stream.skipRawData(id);
        stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
        
        stream << (double)value.toNumber();
    }
}

#endif
