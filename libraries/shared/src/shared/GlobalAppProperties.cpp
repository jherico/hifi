//
//  Created by Bradley Austin Davis on 2016/11/29
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GlobalAppProperties.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QVariant>

namespace hifi { namespace properties {

    const char* CRASHED = "com.highfidelity.crashed";
    const char* STEAM = "com.highfidelity.launchedFromSteam";
    const char* LOGGER = "com.highfidelity.logger";
    const char* OCULUS_STORE = "com.highfidelity.oculusStore";
    const char* TEST_ENABLED = "com.highfidelity.test";
    const char* TEST_SCRIPT = "com.highfidelity.test.script";
    const char* TEST_TRACE = "com.highfidelity.test.trace";
    const char* HMD = "com.highfidelity.hmd";
    const char* APP_LOCAL_DATA_PATH = "com.highfidelity.appLocalDataPath";

    namespace gl {
        const char* BACKEND = "com.highfidelity.gl.backend";
        const char* MAKE_PROGRAM_CALLBACK = "com.highfidelity.gl.makeProgram";
        const char* PRIMARY_CONTEXT = "com.highfidelity.gl.primaryContext";
    }

    bool asBool(const char* flag) {
        auto variant = qApp->property(flag);
        return variant.isValid() && variant.toBool();
    }

} }
