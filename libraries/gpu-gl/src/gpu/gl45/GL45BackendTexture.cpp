//
//  GL45BackendTexture.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/19/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL45Backend.h"

#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <unordered_map>
#include <glm/gtx/component_wise.hpp>

#include <QtCore/QThread>

#include "../gl/GLTexelFormat.h"

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gl45;

#define SPARSE_TEXTURES 1

using GL45Texture = GL45Backend::GL45Texture;

GLTexture* GL45Backend::syncGPUObject(const TexturePointer& texture, bool transfer) {
    return GL45Texture::sync<GL45Texture>(*this, texture, transfer);
}

void serverWait() {
    auto fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    assert(fence);
    glWaitSync(fence, 0, GL_TIMEOUT_IGNORED);
    glDeleteSync(fence);
}

void clientWait() {
    auto fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    assert(fence);
    auto result = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
    while (GL_TIMEOUT_EXPIRED == result || GL_WAIT_FAILED == result) {
        // Minimum sleep
        QThread::usleep(1);
        result = glClientWaitSync(fence, 0, 0);
    }
    glDeleteSync(fence);
}

TransferState::TransferState(GLTexture& texture) : _texture(texture) {
}

void TransferState::updateSparse() {
    ivec3 pageSize;
    _internalFormat = gl::GLTexelFormat::evalGLTexelFormat(_texture._gpuObject.getTexelFormat(), _texture._gpuObject.getTexelFormat()).internalFormat;
    glGetInternalformativ(_texture._target, _internalFormat, GL_VIRTUAL_PAGE_SIZE_X_ARB, 1, &pageSize.x);
    glGetInternalformativ(_texture._target, _internalFormat, GL_VIRTUAL_PAGE_SIZE_Y_ARB, 1, &pageSize.y);
    glGetInternalformativ(_texture._target, _internalFormat, GL_VIRTUAL_PAGE_SIZE_Z_ARB, 1, &pageSize.z);
    _pageSize = uvec3(pageSize);
    GLint maxSparseLevel;
    glGetTextureParameterIiv(_texture._id, GL_NUM_SPARSE_LEVELS_ARB, &maxSparseLevel);
    if (maxSparseLevel == 0) {
        _maxSparseLevel = 0xFFFF;
    }
}

void TransferState::updateMip() {
    _mipDimensions = _texture._gpuObject.evalMipDimensions(_mipLevel);
    _mipOffset = uvec3();
    if (!_texture._gpuObject.isStoredMipFaceAvailable(_mipLevel, _face)) {
        _srcPointer = nullptr;
        return;
    }

    auto mip = _texture._gpuObject.accessStoredMipFace(_mipLevel, _face);
    _texelFormat = gl::GLTexelFormat::evalGLTexelFormat(_texture._gpuObject.getTexelFormat(), mip->getFormat());
    _srcPointer = mip->readData();
    _bytesPerLine = (uint32_t)mip->getSize() / _mipDimensions.y;
    _bytesPerPixel = _bytesPerLine / _mipDimensions.x;
}

bool TransferState::increment() {
    if ((_mipOffset.x + _pageSize.x) < _mipDimensions.x) {
        _mipOffset.x += _pageSize.x;
        return true;
    }

    if ((_mipOffset.y + _pageSize.y) < _mipDimensions.y) {
        _mipOffset.x = 0;
        _mipOffset.y += _pageSize.y;
        return true;
    }

    if (_mipOffset.z + _pageSize.z < _mipDimensions.z) {
        _mipOffset.x = 0;
        _mipOffset.y = 0;
        ++_mipOffset.z;
        return true;
    }

    // Done with this mip?, move on to the next mip 
    if (_mipLevel + 1 < _texture.usedMipLevels()) {
        _mipOffset = uvec3(0);
        ++_mipLevel;
        updateMip();
        return true;
    }

    // Done with this face?  Move on to the next
    if (_face + 1 < ((_texture._target == GL_TEXTURE_CUBE_MAP) ? GLTexture::CUBE_NUM_FACES : 1)) {
        ++_face;
        _mipOffset = uvec3(0);
        _mipLevel = 0;
        updateMip();
        return true;
    }

    return false;
}

void TransferState::populatePage(uint8_t* dst) {
    uvec3 pageSize = currentPageSize();
    for (uint32_t y = 0; y < pageSize.y; ++y) {
        uint32_t srcOffset = (_bytesPerLine * (_mipOffset.y + y)) + (_bytesPerPixel * _mipOffset.x);
        uint32_t dstOffset = (_bytesPerPixel * pageSize.x) * y;
        memcpy(dst + dstOffset, _srcPointer + srcOffset, pageSize.x * _bytesPerPixel);
    }
}

uvec3 TransferState::currentPageSize() const {
    return glm::clamp(_mipDimensions - _mipOffset, uvec3(1), _pageSize);
}

GLuint GL45Texture::allocate(const Texture& texture) {
    GLuint result;
    glCreateTextures(getGLTextureType(texture), 1, &result);
    return result;
}

GLuint GL45Backend::getTextureID(const TexturePointer& texture, bool transfer) {
    return GL45Texture::getId<GL45Texture>(*this, texture, transfer);
}

GL45Texture::GL45Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, bool transferrable)
    : GLTexture(backend, texture, allocate(texture), transferrable), _transferState(*this) {

#if SPARSE_TEXTURES
    if (transferrable) {
        glTextureParameteri(_id, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
    }
#endif
}

GL45Texture::GL45Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, GLTexture* original)
    : GLTexture(backend, texture, allocate(texture), original), _transferState(*this) { }

GL45Texture::~GL45Texture() {
    // FIXME do we need to explicitly deallocate the virtual memory here?
    //if (_transferrable) {
    //    for (uint16_t mipLevel = 0; mipLevel < usedMipLevels(); ++i) {
    //        glTexturePageCommitmentEXT(_id, mipLevel, offset.x, offset.y, offset.z, size.x, size.y, size.z, GL_TRUE);
    //    }
    //}
}

void GL45Texture::withPreservedTexture(std::function<void()> f) const {
    f();
}

void GL45Texture::generateMips() const {
    glGenerateTextureMipmap(_id);
    (void)CHECK_GL_ERROR();
}

void GL45Texture::allocateStorage() const {
    GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat());
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, 0);
    glTextureParameteri(_id, GL_TEXTURE_MAX_LEVEL, _maxMip - _minMip);
    if (_gpuObject.getTexelFormat().isCompressed()) {
        qFatal("Compressed textures not yet supported");
    }
    // Get the dimensions, accounting for the downgrade level
    Vec3u dimensions = _gpuObject.evalMipDimensions(_minMip);
    glTextureStorage2D(_id, usedMipLevels(), texelFormat.internalFormat, dimensions.x, dimensions.y);
    (void)CHECK_GL_ERROR();
}

void GL45Texture::updateSize() const {
    setSize(_virtualSize);
    if (!_id) {
        return;
    }

    if (_gpuObject.getTexelFormat().isCompressed()) {
        qFatal("Compressed textures not yet supported");
    }
}

void GL45Texture::startTransfer() {
    Parent::startTransfer();
    _transferState.updateSparse();
    _transferState.updateMip();
}

bool GL45Texture::continueTransfer() {
    static std::vector<uint8_t> buffer;
    if (buffer.empty()) {
        buffer.resize(1024 * 1024);
    }
    uvec3 pageSize = _transferState.currentPageSize();
    uvec3 offset = _transferState._mipOffset;

#if SPARSE_TEXTURES
    if (_transferState._mipLevel <= _transferState._maxSparseLevel) {
        glTexturePageCommitmentEXT(_id, _transferState._mipLevel,
            offset.x, offset.y, _transferState._face,
            pageSize.x, pageSize.y, pageSize.z,
            GL_TRUE);
    }
#endif

    if (_transferState._srcPointer) {
        // Transfer the mip data
        _transferState.populatePage(&buffer[0]);
        if (GL_TEXTURE_2D == _target) {
            glTextureSubImage2D(_id, _transferState._mipLevel,
                offset.x, offset.y,
                pageSize.x, pageSize.y,
                _transferState._texelFormat.format, _transferState._texelFormat.type, &buffer[0]);
        } else if (GL_TEXTURE_CUBE_MAP == _target) {
            auto target = CUBE_FACE_LAYOUT[_transferState._face];
            // DSA ARB does not work on AMD, so use EXT
            // glTextureSubImage3D(_id, mipLevel, 0, 0, face, size.x, size.y, 1, texelFormat.format, texelFormat.type, mip->readData());
            glTextureSubImage2DEXT(_id, target, _transferState._mipLevel,
                offset.x, offset.y,
                pageSize.x, pageSize.y,
                _transferState._texelFormat.format, _transferState._texelFormat.type, &buffer[0]);
        }
    }

    serverWait();
    return _transferState.increment();
}

void GL45Backend::GL45Texture::syncSampler() const {
    const Sampler& sampler = _gpuObject.getSampler();

    const auto& fm = FILTER_MODES[sampler.getFilter()];
    glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, fm.minFilter);
    glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, fm.magFilter);
    
    if (sampler.doComparison()) {
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_FUNC, COMPARISON_TO_GL[sampler.getComparisonFunction()]);
    } else {
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }

    glTextureParameteri(_id, GL_TEXTURE_WRAP_S, WRAP_MODES[sampler.getWrapModeU()]);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_T, WRAP_MODES[sampler.getWrapModeV()]);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_R, WRAP_MODES[sampler.getWrapModeW()]);
    glTextureParameterfv(_id, GL_TEXTURE_BORDER_COLOR, (const float*)&sampler.getBorderColor());
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, (uint16)sampler.getMipOffset());
    glTextureParameterf(_id, GL_TEXTURE_MIN_LOD, (float)sampler.getMinMip());
    glTextureParameterf(_id, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.f : sampler.getMaxMip()));
    glTextureParameterf(_id, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler.getMaxAnisotropy());
}

