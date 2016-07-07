//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL45Backend.h"
#include "../gl/GLBuffer.h"
#include <shared/SmartMemoryPool.h>

using namespace gpu;
using namespace gpu::gl45;

namespace gpu { namespace gl45 { 

class GL45Buffer : public gl::GLPooledBuffer<GL45BufferProvider> {
    using Parent = gpu::gl::GLPooledBuffer<GL45BufferProvider>;
public:
    GL45Buffer(GL45Backend& backend, const Buffer& buffer, GLPooledBuffer* original = nullptr)
        : Parent(backend._bufferPool, buffer, original) { }
};

gl::GLBuffer* GL45Backend::syncGPUObject(const Buffer& buffer, bool allocateOnly) {
    return GL45Buffer::sync<GL45Backend, GL45Buffer>(*this, buffer, allocateOnly);
}

void GL45Backend::syncBufferPool() {
    _bufferPool.sync();
}


} }