//
//  ArrayBufferClass.h
//
//
//  Created by Clement on 7/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ArrayBufferClass_h
#define hifi_ArrayBufferClass_h

#include <QtCore/QObject>
#include <QtQml/QJSEngine>

// FIXME JSENGINE
#if 0

class ArrayBufferClass : public QObject {
    Q_OBJECT
public:
    ArrayBufferClass(ScriptEngine* scriptEngine);
    QJSValue newInstance(qint32 size);
    QJSValue newInstance(const QByteArray& ba);
    
    //QueryFlags queryProperty(const QJSValue& object,
    //                         const QScriptString& name,
    //                         QueryFlags flags, uint* id);
    //QJSValue property(const QJSValue& object,
    //                      const QScriptString& name, uint id);
    //QJSValue::PropertyFlags propertyFlags(const QJSValue& object,
    //                                          const QScriptString& name, uint id);
    
    QString name() const;
    QJSValue prototype() const;
    
    ScriptEngine* getEngine() { return _scriptEngine; }
    
private:
    static QJSValue construct(QScriptContext* context, QJSEngine* engine);
    
    static QJSValue toScriptValue(QJSEngine* eng, const QByteArray& ba);
    static void fromScriptValue(const QJSValue& obj, QByteArray& ba);
    
    QJSValue _proto;
    QJSValue _ctor;
    
    // JS Object attributes
    QString _name;
    int _byteLength;
    
    ScriptEngine* _scriptEngine;
};

#endif

#endif // hifi_ArrayBufferClass_h