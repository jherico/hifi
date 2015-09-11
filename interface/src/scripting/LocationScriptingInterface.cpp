//
//  LocationScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Ryan Huffman on 4/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <AddressManager.h>

#include "LocationScriptingInterface.h"
// FIXME JSENGINE
#if 0

LocationScriptingInterface* LocationScriptingInterface::getInstance() {
    static LocationScriptingInterface sharedInstance;
    return &sharedInstance;
}

QJSValue LocationScriptingInterface::locationGetter(QScriptContext* context, QJSEngine* engine) {
    return engine->newQObject(DependencyManager::get<AddressManager>().data());
}

QJSValue LocationScriptingInterface::locationSetter(QScriptContext* context, QJSEngine* engine) {
    const QVariant& argumentVariant = context->argument(0).toVariant();
    
    // just try and convert the argument to a string, should be a hifi:// address
    QMetaObject::invokeMethod(DependencyManager::get<AddressManager>().data(), "handleLookupString",
                              Q_ARG(const QString&, argumentVariant.toString()));
    
    return QJSValue::UndefinedValue;
}
#endif
