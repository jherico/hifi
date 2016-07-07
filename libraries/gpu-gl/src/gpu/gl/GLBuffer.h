//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLBuffer_h
#define hifi_gpu_gl_GLBuffer_h

#include "GLShared.h"
#include <unordered_set>

namespace gpu { namespace gl {

class GLBuffer : public GLObjectRef<Buffer> {
    using Parent = GLObjectRef<Buffer>;
    friend class GLBackend;
    static std::unordered_set<GLBuffer*> _activeBuffers;
public:
    template <typename GLBackendType, typename GLBufferType>
    static GLBufferType* sync(GLBackendType& backend, const Buffer& buffer, bool allocateOnly) {
        GLBufferType* object = Backend::getGPUObject<GLBufferType>(buffer);
        // Has the storage size changed?
        if (!object || object->_stamp != buffer.getSysmem().getStamp()) {
            object = new GLBufferType(backend, buffer, object);
        }

        if (!allocateOnly && object->dirty()) {
            object->transfer();
        }

        return object;
    }

    const GLuint _size;
    const Stamp _stamp;

    ~GLBuffer() {
        _activeBuffers.erase(this);
    }

    virtual void transfer() = 0;
    virtual bool dirty() = 0;

    virtual void bind(GLenum target) = 0;
    virtual void bindBase(GLenum target, GLuint index) = 0;
    virtual void bindRange(GLenum target, GLuint index, GLintptr size, GLsizeiptr offset) = 0;

    virtual Offset offset() = 0;

    static void destroyAll();
protected:
    
    GLBuffer(const Buffer& buffer) :
        Parent(buffer),
        _size((GLuint)buffer._sysmem.getSize()),
        _stamp(buffer._sysmem.getStamp()) {
        _activeBuffers.insert(this);
    }
};


template <typename MemoryProviderType>
class GLPooledBuffer : public GLBuffer {
    using Parent = GLBuffer;
protected:
    using Pool = hifi::memory::SmartMemoryPool<MemoryProviderType>;
    using Handle = hifi::memory::Handle;

    GLPooledBuffer(Pool& pool, const Buffer& buffer, GLPooledBuffer* original = nullptr)
        : Parent(buffer), _pool(pool), _handle(pool.allocate(_size))
    { 
        if (original && original->_size && original->_handle) {
            _sourceSize = original->_size;
            std::swap(_sourceHandle, const_cast<Handle&>(original->_handle));
        }
        Backend::setGPUObject(buffer, this);
    }

    Pool& _pool;
    const Handle _handle;
    Handle _sourceHandle { 0 };
    GLuint _sourceSize { 0 };
public:
    ~GLPooledBuffer() {
        if (_sourceHandle) {
            _pool.free(_sourceHandle);
        }
        if (_handle) {
            _pool.free(_handle);
        }
    }

    void bind(GLenum target) override {
        gl::GLBackend::_bufferState.bind(target, _pool.getProvider()._buffer);
    }

    void bindBase(GLenum target, GLuint index) override {
        gl::GLBackend::_bufferState.bindRange(target, index, _pool.getProvider()._buffer, _pool.offset(_handle), _size);
    }

    void bindRange(GLenum target, GLuint index, GLintptr offset, GLsizeiptr size) override {
        gl::GLBackend::_bufferState.bindRange(target, index, _pool.getProvider()._buffer, offset + _pool.offset(_handle), size);
    }

    Offset offset() override final {
        Offset result = static_cast<uint32_t>(_pool.offset(_handle));
        assert(result != hifi::memory::INVALID_OFFSET);
        return result;
    }

    bool dirty() override final {
        return (_sourceHandle != 0 || 0 != (_gpuObject._flags & Buffer::DIRTY));
    }

    void transfer() override {
        if (_sourceHandle) {
            _pool.copy(_handle, 0, _sourceHandle, 0, std::min(_sourceSize, _size));
            _pool.free(_sourceHandle);
            _sourceHandle = 0;
        }

        Size offset;
        Size size;
        Size currentPage { 0 };
        auto data = _gpuObject.getSysmem().readData();
        while (_gpuObject.getNextTransferBlock(offset, size, currentPage)) {
            _pool.update(_handle, offset, size, data + offset);
        }
        (void)CHECK_GL_ERROR();
        _gpuObject._flags &= ~Buffer::DIRTY;
    }
};


} }

#endif
