//
//  HMDScriptingInterface.h
//  interface/src/scripting
//
//  Created by Thijs Wenker on 1/12/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HMDScriptingInterface_h
#define hifi_HMDScriptingInterface_h

#include <GLMHelpers.h>

#include "Application.h"

class HMDScriptingInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool magnifier READ getMagnifier)
    Q_PROPERTY(bool active READ isHMDMode)
public:
    static HMDScriptingInterface& getInstance();

    // FIXME JSENGINE
#if 0
    static QJSValue getHUDLookAtPosition2D(QScriptContext* context, QJSEngine* engine);
    static QJSValue getHUDLookAtPosition3D(QScriptContext* context, QJSEngine* engine);
#endif

public slots:
    void toggleMagnifier() { Application::getInstance()->getApplicationCompositor().toggleMagnifier(); };

private:
    HMDScriptingInterface() {};
    bool getMagnifier() const { return Application::getInstance()->getApplicationCompositor().hasMagnifier(); };
    bool isHMDMode() const { return Application::getInstance()->isHMDMode(); }

    bool getHUDLookAtPosition3D(glm::vec3& result) const;

};

#endif // hifi_HMDScriptingInterface_h
