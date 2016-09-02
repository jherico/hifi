//
//  Created by Bradley Austin Davis on 2016/04/03
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLTextureTransfer.h"

#include <gl/GLHelpers.h>
#include <gl/Context.h>

#include "GLShared.h"
#include "GLTexture.h"

#ifdef HAVE_NSIGHT
#include "nvToolsExt.h"
std::unordered_map<TexturePointer, nvtxRangeId_t> _map;
#endif

//#define TEXTURE_TRANSFER_PBOS

#ifdef TEXTURE_TRANSFER_PBOS
#define TEXTURE_TRANSFER_BLOCK_SIZE (64 * 1024)
#define TEXTURE_TRANSFER_PBO_COUNT 128
#endif


using namespace gpu;
using namespace gpu::gl;

GLTextureTransferHelper::GLTextureTransferHelper() {
#ifdef THREADED_TEXTURE_TRANSFER
    setObjectName("TextureTransferThread");
    _context.create();
    initialize(true, QThread::LowPriority);
    // Clean shutdown on UNIX, otherwise _canvas is freed early
    connect(qApp, &QCoreApplication::aboutToQuit, [&] { terminate(); });
#endif
}

GLTextureTransferHelper::~GLTextureTransferHelper() {
#ifdef THREADED_TEXTURE_TRANSFER
    if (isStillRunning()) {
        terminate();
    }
#endif
}

void GLTextureTransferHelper::transferTexture(const gpu::TexturePointer& texturePointer) {
    GLTexture* object = Backend::getGPUObject<GLTexture>(*texturePointer);

#ifdef THREADED_TEXTURE_TRANSFER
    Backend::incrementTextureGPUTransferCount();
    object->setSyncState(GLSyncState::Pending);
    Lock lock(_mutex);
    _pendingTextures.push_back(texturePointer);
#else
    for (object->startTransfer(); object->continueTransfer(); ) { }
    object->finishTransfer();
    object->_contentStamp = texturePointer->getDataStamp();
    object->setSyncState(GLSyncState::Transferred);
#endif
}

void GLTextureTransferHelper::setup() {
#ifdef THREADED_TEXTURE_TRANSFER
    _context.makeCurrent();
    glCreateRenderbuffers(1, &_drawRenderbuffer);
    glNamedRenderbufferStorage(_drawRenderbuffer, GL_RGBA8, 128, 128);
    glCreateFramebuffers(1, &_drawFramebuffer);
    glNamedFramebufferRenderbuffer(_drawFramebuffer, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _drawRenderbuffer);
    glCreateFramebuffers(1, &_readFramebuffer);
#ifdef TEXTURE_TRANSFER_PBOS
    std::array<GLuint, TEXTURE_TRANSFER_PBO_COUNT> pbos;
    glCreateBuffers(TEXTURE_TRANSFER_PBO_COUNT, &pbos[0]);
    for (uint32_t i = 0; i < TEXTURE_TRANSFER_PBO_COUNT; ++i) {
        TextureTransferBlock newBlock;
        newBlock._pbo = pbos[i];
        glNamedBufferStorage(newBlock._pbo, TEXTURE_TRANSFER_BLOCK_SIZE, 0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        newBlock._mapped = glMapNamedBufferRange(newBlock._pbo, 0, TEXTURE_TRANSFER_BLOCK_SIZE, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        _readyQueue.push(newBlock);
    }
#endif
#endif
}

void GLTextureTransferHelper::shutdown() {
#ifdef THREADED_TEXTURE_TRANSFER
    _context.makeCurrent();

    glNamedFramebufferRenderbuffer(_drawFramebuffer, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, 0);
    glDeleteFramebuffers(1, &_drawFramebuffer);
    _drawFramebuffer = 0;
    glDeleteFramebuffers(1, &_readFramebuffer);
    _readFramebuffer = 0;

    glNamedFramebufferTexture(_readFramebuffer, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0);
    glDeleteRenderbuffers(1, &_drawRenderbuffer);
    _drawRenderbuffer = 0;
#endif
}

bool GLTextureTransferHelper::process() {
#ifdef THREADED_TEXTURE_TRANSFER
    // Take any new textures off the queue
    TextureList newTransferTextures;
    {
        Lock lock(_mutex);
        newTransferTextures.swap(_pendingTextures);
    }

    if (!newTransferTextures.empty()) {
        for (auto& texturePointer : newTransferTextures) {
#ifdef HAVE_NSIGHT
            _map[texturePointer] = nvtxRangeStart("TextureTansfer");
#endif
            GLTexture* object = Backend::getGPUObject<GLTexture>(*texturePointer);
            object->startTransfer();
            _transferringTextures.push_back(texturePointer);
            _textureIterator = _transferringTextures.begin();
        }
    }

    // No transfers in progress, sleep
    if (_transferringTextures.empty()) {
        QThread::usleep(1);
        return true;
    }

    static auto lastReport = usecTimestampNow();
    auto now = usecTimestampNow();
    auto lastReportInterval = now - lastReport;
    if (lastReportInterval > USECS_PER_SECOND * 4) {
        lastReport = now;
        qDebug() << "Texture list " << _transferringTextures.size();
    }

    for (auto _textureIterator = _transferringTextures.begin(); _textureIterator != _transferringTextures.end();) {
        auto texture = *_textureIterator;
        GLTexture* gltexture = Backend::getGPUObject<GLTexture>(*texture);
        if (gltexture->continueTransfer()) {
            ++_textureIterator;
            continue;
        }

        gltexture->finishTransfer();
        glNamedFramebufferTexture(_readFramebuffer, GL_COLOR_ATTACHMENT0, gltexture->_id, 0);
        glBlitNamedFramebuffer(_readFramebuffer, _drawFramebuffer, 0, 0, 1, 1, 0, 0, 1, 1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glFinish();
        gltexture->_contentStamp = gltexture->_gpuObject.getDataStamp();
        gltexture->updateSize();
        gltexture->setSyncState(gpu::gl::GLSyncState::Transferred);
        Backend::decrementTextureGPUTransferCount();
#ifdef HAVE_NSIGHT
        // Mark the texture as transferred
        nvtxRangeEnd(_map[texture]);
        _map.erase(texture);
#endif
        _textureIterator = _transferringTextures.erase(_textureIterator);
    }

    if (!_transferringTextures.empty()) {
        // Don't saturate the GPU
        glFinish();
    } else {
        // Don't saturate the CPU
        QThread::msleep(1);
    }
#else
    QThread::msleep(1);
#endif
    return true;
}

#if 0
bool TextureTransferBlock::isSignaled() {
    assert(_fence);
    auto result = glClientWaitSync(_fence, 0, 0);
    if (GL_WAIT_FAILED == result) {
        CHECK_GL_ERROR();
    }
    if (GL_TIMEOUT_EXPIRED == result || GL_WAIT_FAILED == result) {
        return false;
    }
    assert(GL_CONDITION_SATISFIED == result || GL_ALREADY_SIGNALED == result);
    glDeleteSync(_fence);
    _fence = 0;
    return true;
}

void TextureTransferBlock::transfer() {
    _transferCallback();
    _transferCallback = [] {};
}


//BlockQueue GLTextureTransferHelper::getSignaledQueueItems(BlockQueue& sourceQueue) {
//    BlockQueue result;
//    Lock lock(_mutex);
//    while (!sourceQueue.empty() && sourceQueue.front().isSignaled()) {
//        result.push(sourceQueue.front());
//        sourceQueue.pop();
//    }
//    return result;
//}

#define MAX_TRANSFER_BUDGET (USECS_PER_MSEC * 5)
void GLTextureTransferHelper::flushTransfers() {
    //CommandQueue pendingCommands;
    //{
    //    Lock lock(_mutex);
    //    _renderThreadCommands.swap(pendingCommands);
    //}

    //auto start = usecTimestampNow();
    //auto elapsed = 0;
    //while (!pendingCommands.empty() && (elapsed < MAX_TRANSFER_BUDGET)) {
    //    (pendingCommands.front())();
    //    pendingCommands.pop_front();
    //    elapsed = usecTimestampNow() - start;
    //}

    //// Put back any incomplete commands
    //if (!pendingCommands.empty()) {
    //    Lock lock(_mutex);
    //    _renderThreadCommands.insert(_renderThreadCommands.begin(), pendingCommands.begin(), pendingCommands.end());
    //}

    //// Check for any blocks available for putting back in the ready queue
    //BlockQueue queue = getSignaledQueueItems(_transferredQueue);
    //Lock lock(_mutex);
    //while (!queue.empty()) {
    //    auto block = queue.front();
    //    queue.pop();
    //    _readyQueue.push(block);
    //}
}
uint32_t GLTextureTransferHelper::getBlockSize() const {
    return TEXTURE_TRANSFER_BLOCK_SIZE;
}
#endif

