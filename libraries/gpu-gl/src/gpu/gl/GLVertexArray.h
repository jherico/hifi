//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLVertexArray_h
#define hifi_gpu_gl_GLVertexArray_h

#include "GLShared.h"

namespace gpu { namespace gl {

class GLBackend;

class GLVertexArray : public GLObject<VertexArray> {
public:
    static GLVertexArray* sync(GLBackend& backend, const VertexArray& vertexArray);
    static GLuint getId(GLBackend& backend, const VertexArray& vertexArray);
    const Stamp _stamp;

    ~GLVertexArray();

protected:
    static GLuint allocate();
    GLVertexArray(GLBackend& backend, const VertexArray& buffer);
};

} }

#endif
