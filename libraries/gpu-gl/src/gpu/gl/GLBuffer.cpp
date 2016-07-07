//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLBuffer.h"

using namespace gpu;
using namespace gpu::gl;

std::unordered_set<GLBuffer*> GLBuffer::_activeBuffers;

void GLBuffer::destroyAll() {
    std::unordered_set<GLBuffer*> destroyBuffers;
    destroyBuffers.swap(_activeBuffers);
    for (const auto& buffer : destroyBuffers) {
        Backend::setGPUObject<GLBuffer>(buffer->_gpuObject, nullptr);
    }
}
