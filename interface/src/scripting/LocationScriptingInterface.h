//
//  LocationScriptingInterface.h
//  interface/src/scripting
//
//  Created by Ryan Huffman on 4/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LocationScriptingInterface_h
#define hifi_LocationScriptingInterface_h

// FIXME JSENGINE
#if 0
#include <QtQML/QJSEngine>

class LocationScriptingInterface : public QObject {
    Q_OBJECT
public:
    static LocationScriptingInterface* getInstance();

    static QJSValue locationGetter(QScriptContext* context, QJSEngine* engine);
    static QJSValue locationSetter(QScriptContext* context, QJSEngine* engine);
private:
    LocationScriptingInterface() {};
};
#endif

#endif // hifi_LocationScriptingInterface_h
