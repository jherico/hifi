//
//  Created by Bradley Austin Davis on 2016/11/29
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_GlobalAppProperties_h
#define hifi_GlobalAppProperties_h

namespace hifi { namespace properties {

    extern const char* CRASHED;
    extern const char* STEAM;
    extern const char* LOGGER;
    extern const char* OCULUS_STORE;
    extern const char* TEST_ENABLED;
    extern const char* TEST_SCRIPT;
    extern const char* TEST_TRACE;
    extern const char* HMD;
    extern const char* APP_LOCAL_DATA_PATH;

    namespace gl {
        extern const char* BACKEND;
        extern const char* MAKE_PROGRAM_CALLBACK;
        extern const char* PRIMARY_CONTEXT;
    }

    bool asBool(const char* flag);

} }


#endif // hifi_GlobalAppProperties_h
