//
//  GL45BackendTransform.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL45Backend.h"

using namespace gpu;
using namespace gpu::gl45;

void GL45Backend::initTransform() {
    GLuint transformBuffers[3];
    glCreateBuffers(3, transformBuffers);
    _transform._objectBuffer = transformBuffers[0];
    _transform._cameraBuffer = transformBuffers[1];
    _transform._drawCallInfoBuffer = transformBuffers[2];
<<<<<<< HEAD
    glCreateTextures(GL_TEXTURE_BUFFER, 1, &_transform._objectBufferTexture);
=======
    glCreateTextures(GL_TEXTURE_2D, 1, &_transform._objectBufferTexture);
>>>>>>> efcb4ac... working on shader storage buffer objects
    size_t cameraSize = sizeof(TransformStageState::CameraBufferElement);
    while (_transform._cameraUboSize < cameraSize) {
        _transform._cameraUboSize += _uboAlignment;
    }
}

void GL45Backend::transferTransformState(const Batch& batch) const {
    // FIXME not thread safe
    static std::vector<uint8_t> bufferData;
    if (!_transform._cameras.empty()) {
        bufferData.resize(_transform._cameraUboSize * _transform._cameras.size());
        for (size_t i = 0; i < _transform._cameras.size(); ++i) {
            memcpy(bufferData.data() + (_transform._cameraUboSize * i), &_transform._cameras[i], sizeof(TransformStageState::CameraBufferElement));
        }
        glNamedBufferData(_transform._cameraBuffer, bufferData.size(), bufferData.data(), GL_STREAM_DRAW);
    }

    if (!batch._objects.empty()) {
        auto byteSize = batch._objects.size() * sizeof(Batch::TransformObject);
        bufferData.resize(byteSize);
        memcpy(bufferData.data(), batch._objects.data(), byteSize);
        glNamedBufferData(_transform._objectBuffer, bufferData.size(), bufferData.data(), GL_STREAM_DRAW);
    }

    if (!batch._namedData.empty()) {
        bufferData.clear();
        for (auto& data : batch._namedData) {
            auto currentSize = bufferData.size();
            auto bytesToCopy = data.second.drawCallInfos.size() * sizeof(Batch::DrawCallInfo);
            bufferData.resize(currentSize + bytesToCopy);
            memcpy(bufferData.data() + currentSize, data.second.drawCallInfos.data(), bytesToCopy);
            _transform._drawCallInfoOffsets[data.first] = (GLvoid*)currentSize;
        }
        glNamedBufferData(_transform._drawCallInfoBuffer, bufferData.size(), bufferData.data(), GL_STREAM_DRAW);
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, TRANSFORM_OBJECT_SLOT, _transform._objectBuffer);
<<<<<<< HEAD
#else
    glTextureBuffer(_transform._objectBufferTexture, GL_RGBA32F, _transform._objectBuffer);
    glActiveTexture(GL_TEXTURE0 + TRANSFORM_OBJECT_SLOT);
    glBindTexture(GL_TEXTURE_BUFFER, _transform._objectBufferTexture);
#endif
=======
>>>>>>> efcb4ac... working on shader storage buffer objects

    CHECK_GL_ERROR();

    // Make sure the current Camera offset is unknown before render Draw
    _transform._currentCameraOffset = INVALID_OFFSET;
}
