//
//  GL41Backend.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_41_GL41Backend_h
#define hifi_gpu_41_GL41Backend_h

#include <gl/Config.h>

#include "../gl/GLBackend.h"
#include "../gl/GLTexture.h"

#define GPU_CORE_41 410
#define GPU_CORE_43 430

#ifdef Q_OS_MAC
#define GPU_INPUT_PROFILE GPU_CORE_41
#else 
#define GPU_INPUT_PROFILE GPU_CORE_43
#endif

namespace gpu { namespace gl41 {

class GL41BufferProvider : public hifi::memory::Provider {
public:
    GLuint _buffer { 0 };
    // Allocate in 16 MB increments
    static const size_t GPU_MEMORY_INCREMENT = 16 * 1024 * 1024;
    // Start with 64 MB
    static const size_t GPU_MEMORY_INITIAL_SIZE = 64 * 1024 * 1024;

    ~GL41BufferProvider() {
        if (_buffer) {
            glDeleteBuffers(1, &_buffer);
        }
    }

    size_t reallocate(size_t newSize) override {
        if (newSize < GPU_MEMORY_INITIAL_SIZE) {
            newSize = GPU_MEMORY_INITIAL_SIZE;
        } else if (newSize % GPU_MEMORY_INCREMENT) {
            newSize -= newSize % GPU_MEMORY_INCREMENT;
            newSize += GPU_MEMORY_INCREMENT;
        }

        GLuint newBuffer { 0 };
        GLsizei count = 1;
        glCreateBuffers(count, &newBuffer);
        (void)CHECK_GL_ERROR();
        glNamedBufferStorage(newBuffer, newSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
        (void)CHECK_GL_ERROR();

        if (_buffer && _size) {
            glCopyNamedBufferSubData(newBuffer, newBuffer, 0, 0, std::min(newSize, _size));
            (void)CHECK_GL_ERROR();
        }
        std::swap(newBuffer, _buffer);
        std::swap(newSize, _size);
        if (newBuffer) {
            glDeleteBuffers(1, &newBuffer);
            (void)CHECK_GL_ERROR();
        }
        return _size;
    }

    void copy(size_t dstOffset, size_t srcOffset, size_t size) const override {
        assert(validRange(dstOffset, size));
        assert(validRange(srcOffset, size));
        assert(!overlap(size, dstOffset, srcOffset));
        glCopyNamedBufferSubData(_buffer, _buffer, srcOffset, dstOffset, size);
        (void)CHECK_GL_ERROR();
    }

    void update(size_t dstOffset, size_t size, const uint8_t* data) const override {
        assert(validRange(dstOffset, size));
        glNamedBufferSubData(_buffer, dstOffset, size, data);
        (void)CHECK_GL_ERROR();
    }
};

#if 0
glBindBuffer(GL_ARRAY_BUFFER, _buffer);
glBufferData(GL_ARRAY_BUFFER, _size, nullptr, GL_DYNAMIC_DRAW);
glBindBuffer(GL_ARRAY_BUFFER, 0);

if (original && original->_size) {
    (void)CHECK_GL_ERROR();
}
Backend::setGPUObject(buffer, this);

void transfer() override {
    glBindBuffer(GL_ARRAY_BUFFER, _buffer);
    (void)CHECK_GL_ERROR();
    Size offset;
    Size size;
    Size currentPage { 0 };
    auto data = _gpuObject.getSysmem().readData();
    while (_gpuObject.getNextTransferBlock(offset, size, currentPage)) {
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, data + offset);
        (void)CHECK_GL_ERROR();
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    (void)CHECK_GL_ERROR();
    _gpuObject._flags &= ~Buffer::DIRTY;
}
#endif
using GL41BufferPool = hifi::memory::SmartMemoryPool<GL41BufferProvider>;

class GL41Backend : public gl::GLBackend {
    using Parent = gl::GLBackend;
    // Context Backend static interface required
    friend class Context;

public:
    explicit GL41Backend(bool syncCache) : Parent(syncCache) {}
    GL41Backend() : Parent() {}
    ~GL41Backend();

    class GL41Texture : public gpu::gl::GLTexture {
        using Parent = gpu::gl::GLTexture;
        GLuint allocate();
    public:
        GL41Texture(const Texture& buffer, bool transferrable);
        GL41Texture(const Texture& buffer, GL41Texture* original);

    protected:
        void transferMip(uint16_t mipLevel, uint8_t face = 0) const;
        void allocateStorage() const override;
        void updateSize() const override;
        void transfer() const override;
        void syncSampler() const override;
        void generateMips() const override;
        void withPreservedTexture(std::function<void()> f) const override;
    };


protected:
    friend class GL41Buffer;
    GL41BufferPool _bufferPool;
        
    GLuint getFramebufferID(const FramebufferPointer& framebuffer) override;
    gl::GLFramebuffer* syncGPUObject(const Framebuffer& framebuffer) override;

    void syncBufferPool() override;
    gl::GLBuffer* syncGPUObject(const Buffer& buffer, bool allocateOnly = false) override;

    GLuint getTextureID(const TexturePointer& texture, bool needTransfer = true) override;
    gl::GLTexture* syncGPUObject(const TexturePointer& texture, bool sync = true) override;

    GLuint getQueryID(const QueryPointer& query) override;
    gl::GLQuery* syncGPUObject(const Query& query) override;

    // Draw Stage
    void do_draw(Batch& batch, size_t paramOffset) override;
    void do_drawIndexed(Batch& batch, size_t paramOffset) override;
    void do_drawInstanced(Batch& batch, size_t paramOffset) override;
    void do_drawIndexedInstanced(Batch& batch, size_t paramOffset) override;
    void do_multiDrawIndirect(Batch& batch, size_t paramOffset) override;
    void do_multiDrawIndexedIndirect(Batch& batch, size_t paramOffset) override;

    // Input Stage
    void updateInput() override;

    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void transferTransformState(const Batch& batch) const override;
    void initTransform() override;
    void updateTransform(const Batch& batch);
    void resetTransformStage();

    // Output stage
    void do_blit(Batch& batch, size_t paramOffset) override;
};

} }

Q_DECLARE_LOGGING_CATEGORY(gpugl41logging)


#endif
