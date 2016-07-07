//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL41Backend.h"
#include "../gl/GLBuffer.h"

namespace gpu { namespace gl41 {

class GL41Buffer : public gl::GLPooledBuffer<GL41BufferProvider> {
    using Parent = gpu::gl::GLPooledBuffer<GL41BufferProvider>;
public:
    GL41Buffer(GL41Backend& backend, const Buffer& buffer, GL41Buffer* original)
        : Parent(backend._bufferPool, buffer, original) { }
};

gl::GLBuffer* GL41Backend::syncGPUObject(const Buffer& buffer, bool allocateOnly) {
    return GL41Buffer::sync<GL41Backend, GL41Buffer>(*this, buffer, allocateOnly);
}

void GL41Backend::syncBufferPool() {
    _bufferPool.sync();
}

} }