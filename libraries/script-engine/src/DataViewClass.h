//
//  DataViewClass.h
//
//
//  Created by Clement on 7/8/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DataViewClass_h
#define hifi_DataViewClass_h

#include "ArrayBufferViewClass.h"

#if 0

class DataViewClass : public ArrayBufferViewClass {
    Q_OBJECT
public:
    DataViewClass(ScriptEngine* scriptEngine);
    QJSValue newInstance(QJSValue buffer, quint32 byteOffset, quint32 byteLength);
    
    QString name() const;
    QJSValue prototype() const;
    
private:
    static QJSValue construct(QScriptContext* context, QJSEngine* engine);
    
    QJSValue _proto;
    QJSValue _ctor;
    
    QScriptString _name;
};
#endif

#endif // hifi_DataViewClass_h