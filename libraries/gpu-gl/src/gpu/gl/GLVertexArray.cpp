//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLVertexArray.h"
#include "GLBackend.h"

using namespace gpu;
using namespace gpu::gl;

GLuint GLVertexArray::allocate() {
    GLuint result;
    glGenVertexArrays(1, &result);
    return result;
}

GLVertexArray::~GLVertexArray() {
    glDeleteVertexArrays(1, &_id);
}

GLVertexArray::GLVertexArray(GLBackend& backend, const VertexArray& vertexArray) :
GLObject(vertexArray, allocate()), _stamp(vertexArray.getStamp()) {
    Backend::setGPUObject(vertexArray, this);

    glBindVertexArray(_id);
    const auto& format = vertexArray.getFormat();
    const Stream::Format::AttributeMap& attributes = format->getAttributes();
    const auto& inputChannels = format->getChannels();
    const auto& stream = vertexArray.getStream();
    const Buffers& buffers = stream.getBuffers();
    const Offsets& offsets = stream.getOffsets();
    const Offsets& strides = stream.getStrides();

    GLBackend::InputStageState::ActivationCache newActivation;
    for (auto& it : attributes) {
        const Stream::Attribute& attrib = (it).second;
        uint8_t locationCount = attrib._element.getLocationCount();
        for (int i = 0; i < locationCount; ++i) {
            newActivation.set(attrib._slot + i);
        }
    }

    // Manage Activation what was and what is expected now
    for (unsigned int i = 0; i < newActivation.size(); i++) {
        if (newActivation[i]) {
            glEnableVertexAttribArray(i);
        }
    }

    for (auto& channelIt : inputChannels) {
        const Stream::Format::ChannelMap::value_type::second_type& channel = (channelIt).second;
        if ((channelIt).first < buffers.size()) {
            int bufferNum = (channelIt).first;
            const Buffer& buffer = *buffers[bufferNum];
            GLuint vbo = backend.getBufferID(buffer);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            for (unsigned int i = 0; i < channel._slots.size(); i++) {
                const Stream::Attribute& attrib = attributes.at(channel._slots[i]);
                GLuint slot = attrib._slot;
                GLuint count = attrib._element.getLocationScalarCount();
                uint8_t locationCount = attrib._element.getLocationCount();
                GLenum type = gl::ELEMENT_TYPE_TO_GL[attrib._element.getType()];
                // GLenum perLocationStride = strides[bufferNum];
                GLenum perLocationStride = attrib._element.getLocationSize();
                GLuint stride = (GLuint)strides[bufferNum];
                GLuint pointer = (GLuint)(attrib._offset + offsets[bufferNum]);
                GLboolean isNormalized = attrib._element.isNormalized();

                for (size_t locNum = 0; locNum < locationCount; ++locNum) {
                    glVertexAttribPointer(slot + (GLuint)locNum, count, type, isNormalized, stride,
                        reinterpret_cast<GLvoid*>(pointer + perLocationStride * (GLuint)locNum));
                    glVertexAttribDivisor(slot + (GLuint)locNum, attrib._frequency);
                }
                (void)CHECK_GL_ERROR();
            }
        }
    }
    glBindVertexArray(backend._input._defaultVAO);
}

GLVertexArray* GLVertexArray::sync(GLBackend& backend, const VertexArray& vertexArray) {
    GLVertexArray* object = Backend::getGPUObject<GLVertexArray>(vertexArray);

    // Has the vao changed?
    if (!object || object->_stamp != vertexArray.getStamp()) {
        object = new GLVertexArray(backend, vertexArray);
    }

    return object;
}

GLuint GLVertexArray::getId(GLBackend& backend, const VertexArray& vertexArray) {
    GLVertexArray* vao = sync(backend, vertexArray);
    if (vao) {
        return vao->_id;
    } else {
        return 0;
    }
}
