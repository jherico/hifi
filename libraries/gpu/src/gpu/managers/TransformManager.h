//
//  Created by Bradley Austin Davis on 2018/09/23
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef GPU_TRANSFORM_MANAGER_H
#define GPU_TRANSFORM_MANAGER_H

#include "Manager.h"
#include <Transform.h>

namespace gpu { namespace manager {

class TransformManager : public Manager<Transform> {
    using Parent = Manager<Transform>;

protected:
    TransformManager() {};
};



}}

#endif
