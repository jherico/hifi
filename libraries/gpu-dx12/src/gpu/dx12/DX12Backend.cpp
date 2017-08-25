//
//  DX12Backend.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "DX12Backend.h"

#include <mutex>
#include <queue>
#include <list>
#include <functional>

#if defined(NSIGHT_FOUND)
#include "nvToolsExt.h"
#endif

#include <shared/GlobalAppProperties.h>
#include <GPUIdent.h>
#include <QtCore/QProcessEnvironment>

using namespace gpu;
using namespace gpu::dx12;

static DX12Backend* INSTANCE{ nullptr };

BackendPointer DX12Backend::createBackend() {
    std::shared_ptr<DX12Backend> result = std::make_shared<DX12Backend>();
    INSTANCE = result.get();
    void* voidInstance = &(*result);
    qApp->setProperty(hifi::properties::dx12::BACKEND, QVariant::fromValue(voidInstance));
    return result;
}

DX12Backend& getBackend() {
    if (!INSTANCE) {
        INSTANCE = static_cast<DX12Backend*>(qApp->property(hifi::properties::dx12::BACKEND).value<void*>());
    }
    return *INSTANCE;
}

bool DX12Backend::makeProgram(Shader& shader, const Shader::BindingSet& slotBindings) {
    return false;
}

DX12Backend::CommandCall DX12Backend::_commandCalls[Batch::NUM_COMMANDS] = 
{
    (&::gpu::dx12::DX12Backend::do_draw),
    (&::gpu::dx12::DX12Backend::do_drawIndexed),
    (&::gpu::dx12::DX12Backend::do_drawInstanced),
    (&::gpu::dx12::DX12Backend::do_drawIndexedInstanced),
    (&::gpu::dx12::DX12Backend::do_multiDrawIndirect),
    (&::gpu::dx12::DX12Backend::do_multiDrawIndexedIndirect),

    (&::gpu::dx12::DX12Backend::do_setInputFormat),
    (&::gpu::dx12::DX12Backend::do_setInputBuffer),
    (&::gpu::dx12::DX12Backend::do_setIndexBuffer),
    (&::gpu::dx12::DX12Backend::do_setIndirectBuffer),

    (&::gpu::dx12::DX12Backend::do_setModelTransform),
    (&::gpu::dx12::DX12Backend::do_setViewTransform),
    (&::gpu::dx12::DX12Backend::do_setProjectionTransform),
    (&::gpu::dx12::DX12Backend::do_setViewportTransform),
    (&::gpu::dx12::DX12Backend::do_setDepthRangeTransform),

    (&::gpu::dx12::DX12Backend::do_setPipeline),
    (&::gpu::dx12::DX12Backend::do_setStateBlendFactor),
    (&::gpu::dx12::DX12Backend::do_setStateScissorRect),

    (&::gpu::dx12::DX12Backend::do_setUniformBuffer),
    (&::gpu::dx12::DX12Backend::do_setResourceBuffer),
    (&::gpu::dx12::DX12Backend::do_setResourceTexture),

    (&::gpu::dx12::DX12Backend::do_setFramebuffer),
    (&::gpu::dx12::DX12Backend::do_clearFramebuffer),
    (&::gpu::dx12::DX12Backend::do_blit),
    (&::gpu::dx12::DX12Backend::do_generateTextureMips),

    (&::gpu::dx12::DX12Backend::do_beginQuery),
    (&::gpu::dx12::DX12Backend::do_endQuery),
    (&::gpu::dx12::DX12Backend::do_getQuery),

    (&::gpu::dx12::DX12Backend::do_resetStages),
    
    (&::gpu::dx12::DX12Backend::do_disableContextViewCorrection),
    (&::gpu::dx12::DX12Backend::do_restoreContextViewCorrection),
    (&::gpu::dx12::DX12Backend::do_disableContextStereo),
    (&::gpu::dx12::DX12Backend::do_restoreContextStereo),

    (&::gpu::dx12::DX12Backend::do_runLambda),

    (&::gpu::dx12::DX12Backend::do_startNamedCall),
    (&::gpu::dx12::DX12Backend::do_stopNamedCall),

    (&::gpu::dx12::DX12Backend::do_glUniform1i),
    (&::gpu::dx12::DX12Backend::do_glUniform1f),
    (&::gpu::dx12::DX12Backend::do_glUniform2f),
    (&::gpu::dx12::DX12Backend::do_glUniform3f),
    (&::gpu::dx12::DX12Backend::do_glUniform4f),
    (&::gpu::dx12::DX12Backend::do_glUniform3fv),
    (&::gpu::dx12::DX12Backend::do_glUniform4fv),
    (&::gpu::dx12::DX12Backend::do_glUniform4iv),
    (&::gpu::dx12::DX12Backend::do_glUniformMatrix3fv),
    (&::gpu::dx12::DX12Backend::do_glUniformMatrix4fv),

    (&::gpu::dx12::DX12Backend::do_glColor4f),

    (&::gpu::dx12::DX12Backend::do_pushProfileRange),
    (&::gpu::dx12::DX12Backend::do_popProfileRange),
};

void DX12Backend::init() {
}

DX12Backend::DX12Backend() {
    //flushBuffer(_pipeline._cameraCorrectionBuffer._buffer);
    //glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &_uboAlignment);
}

DX12Backend::~DX12Backend() {
    killInput();
    killTransform();
}

#if 0
void DX12Backend::renderPassTransfer(const Batch& batch) {
    const size_t numCommands = batch.getCommands().size();
    const Batch::Commands::value_type* command = batch.getCommands().data();
    const Batch::CommandOffsets::value_type* offset = batch.getCommandOffsets().data();

    _inRenderTransferPass = true;
    { // Sync all the buffers
        PROFILE_RANGE(render_gpu_dx_detail, "syncGPUBuffer");

        for (auto& cached : batch._buffers._items) {
            if (cached._data) {
                syncGPUObject(*cached._data);
            }
        }
    }

    { // Sync all the transform states
        PROFILE_RANGE(render_gpu_dx_detail, "syncCPUTransform");
        _transform._cameras.clear();
        _transform._cameraOffsets.clear();

        for (_commandIndex = 0; _commandIndex < numCommands; ++_commandIndex) {
            switch (*command) {
                case Batch::COMMAND_draw:
                case Batch::COMMAND_drawIndexed:
                case Batch::COMMAND_drawInstanced:
                case Batch::COMMAND_drawIndexedInstanced:
                case Batch::COMMAND_multiDrawIndirect:
                case Batch::COMMAND_multiDrawIndexedIndirect:
                    _transform.preUpdate(_commandIndex, _stereo);
                    break;

                case Batch::COMMAND_disableContextStereo:
                    _stereo._contextDisable = true;
                    break;

                case Batch::COMMAND_restoreContextStereo:
                    _stereo._contextDisable = false;
                    break;

                case Batch::COMMAND_setViewportTransform:
                case Batch::COMMAND_setViewTransform:
                case Batch::COMMAND_setProjectionTransform: {
                    CommandCall call = _commandCalls[(*command)];
                    (this->*(call))(batch, *offset);
                    break;
                }

                default:
                    break;
            }
            command++;
            offset++;
        }
    }

    { // Sync the transform buffers
        PROFILE_RANGE(render_gpu_dx_detail, "syncGPUTransform");
        transferTransformState(batch);
    }

    _inRenderTransferPass = false;
}

void DX12Backend::renderPassDraw(const Batch& batch) {
    _currentDraw = -1;
    _transform._camerasItr = _transform._cameraOffsets.begin();
    const size_t numCommands = batch.getCommands().size();
    const Batch::Commands::value_type* command = batch.getCommands().data();
    const Batch::CommandOffsets::value_type* offset = batch.getCommandOffsets().data();
    for (_commandIndex = 0; _commandIndex < numCommands; ++_commandIndex) {
        switch (*command) {
            // Ignore these commands on this pass, taken care of in the transfer pass
            // Note we allow COMMAND_setViewportTransform to occur in both passes
            // as it both updates the transform object (and thus the uniforms in the 
            // UBO) as well as executes the actual viewport call
            case Batch::COMMAND_setModelTransform:
            case Batch::COMMAND_setViewTransform:
            case Batch::COMMAND_setProjectionTransform:
                break;

            case Batch::COMMAND_draw:
            case Batch::COMMAND_drawIndexed:
            case Batch::COMMAND_drawInstanced:
            case Batch::COMMAND_drawIndexedInstanced:
            case Batch::COMMAND_multiDrawIndirect:
            case Batch::COMMAND_multiDrawIndexedIndirect: {
                // updates for draw calls
                ++_currentDraw;
                updateInput();
                updateTransform(batch);
                updatePipeline();

                CommandCall call = _commandCalls[(*command)];
                (this->*(call))(batch, *offset);
                break;
            }
            default: {
                CommandCall call = _commandCalls[(*command)];
                (this->*(call))(batch, *offset);
                break;
            }
        }

        command++;
        offset++;
    }
}

void DX12Backend::render(const Batch& batch) {
    _transform._skybox = _stereo._skybox = batch.isSkyboxEnabled();
    // Allow the batch to override the rendering stereo settings
    // for things like full framebuffer copy operations (deferred lighting passes)
    bool savedStereo = _stereo._enable;
    if (!batch.isStereoEnabled()) {
        _stereo._enable = false;
    }
    
    {
        PROFILE_RANGE(render_gpu_dx_detail, "Transfer");
        renderPassTransfer(batch);
    }

#ifdef GPU_STEREO_DRAWCALL_INSTANCED
    if (_stereo.isStereo()) {
        glEnable(GL_CLIP_DISTANCE0);
    }
#endif
    {
        PROFILE_RANGE(render_gpu_dx_detail, _stereo.isStereo() ? "Render Stereo" : "Render");
        renderPassDraw(batch);
    }
#ifdef GPU_STEREO_DRAWCALL_INSTANCED
    if (_stereo.isStereo()) {
        glDisable(GL_CLIP_DISTANCE0);
    }
#endif
    // Restore the saved stereo state for the next batch
    _stereo._enable = savedStereo;
}


void DX12Backend::syncCache() {
    PROFILE_RANGE(render_gpu_dx_detail, __FUNCTION__);

    syncTransformStateCache();
    syncPipelineStateCache();
    syncInputStateCache();
    syncOutputStateCache();
}

#ifdef GPU_STEREO_DRAWCALL_DOUBLED
void DX12Backend::setupStereoSide(int side) {
    ivec4 vp = _transform._viewport;
    vp.z /= 2;
    glViewport(vp.x + side * vp.z, vp.y, vp.z, vp.w);


#ifdef GPU_STEREO_CAMERA_BUFFER
#ifdef GPU_STEREO_DRAWCALL_DOUBLED
    glVertexAttribI1i(14, side);
#endif
#else
    _transform.bindCurrentCamera(side);
#endif

}
#else
#endif

void DX12Backend::do_resetStages(const Batch& batch, size_t paramOffset) {
    resetStages();
}

void DX12Backend::do_disableContextViewCorrection(const Batch& batch, size_t paramOffset) {
    _transform._viewCorrectionEnabled = false;
}

void DX12Backend::do_restoreContextViewCorrection(const Batch& batch, size_t paramOffset) {
    _transform._viewCorrectionEnabled = true;
}

void DX12Backend::do_disableContextStereo(const Batch& batch, size_t paramOffset) {

}

void DX12Backend::do_restoreContextStereo(const Batch& batch, size_t paramOffset) {

}

void DX12Backend::do_runLambda(const Batch& batch, size_t paramOffset) {
    std::function<void()> f = batch._lambdas.get(batch._params[paramOffset]._uint);
    f();
}

void DX12Backend::do_startNamedCall(const Batch& batch, size_t paramOffset) {
    batch._currentNamedCall = batch._names.get(batch._params[paramOffset]._uint);
    _currentDraw = -1;
}

void DX12Backend::do_stopNamedCall(const Batch& batch, size_t paramOffset) {
    batch._currentNamedCall.clear();
}

void DX12Backend::resetStages() {
    resetInputStage();
    resetPipelineStage();
    resetTransformStage();
    resetUniformStage();
    resetResourceStage();
    resetOutputStage();
    resetQueryStage();

    (void) CHECK_GL_ERROR();
}


void DX12Backend::do_pushProfileRange(const Batch& batch, size_t paramOffset) {
    if (trace_render_gpu_dx_detail().isDebugEnabled()) {
        auto name = batch._profileRanges.get(batch._params[paramOffset]._uint);
        profileRanges.push_back(name);
#if defined(NSIGHT_FOUND)
        nvtxRangePush(name.c_str());
#endif
    }
}

void DX12Backend::do_popProfileRange(const Batch& batch, size_t paramOffset) {
    if (trace_render_gpu_dx_detail().isDebugEnabled()) {
        profileRanges.pop_back();
#if defined(NSIGHT_FOUND)
        nvtxRangePop();
#endif
    }
}

// TODO: As long as we have gl calls explicitely issued from interface
// code, we need to be able to record and batch these calls. THe long 
// term strategy is to get rid of any GL calls in favor of the HIFI GPU API

// As long as we don;t use several versions of shaders we can avoid this more complex code path
#ifdef GPU_STEREO_CAMERA_BUFFER
#define GET_UNIFORM_LOCATION(shaderUniformLoc) ((_pipeline._programShader) ? _pipeline._programShader->getUniformLocation(shaderUniformLoc, (GLShader::Version) isStereo()) : -1)
#else
#define GET_UNIFORM_LOCATION(shaderUniformLoc) shaderUniformLoc
#endif

void DX12Backend::do_glUniform1i(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();

    glUniform1i(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 1]._int),
        batch._params[paramOffset + 0]._int);
    (void)CHECK_GL_ERROR();
}

void DX12Backend::do_glUniform1f(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();

    glUniform1f(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 1]._int),
        batch._params[paramOffset + 0]._float);
    (void)CHECK_GL_ERROR();
}

void DX12Backend::do_glUniform2f(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform2f(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 2]._int),
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float);
    (void)CHECK_GL_ERROR();
}

void DX12Backend::do_glUniform3f(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform3f(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 3]._int),
        batch._params[paramOffset + 2]._float,
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float);
    (void)CHECK_GL_ERROR();
}

void DX12Backend::do_glUniform4f(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform4f(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 4]._int),
        batch._params[paramOffset + 3]._float,
        batch._params[paramOffset + 2]._float,
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float);
    (void)CHECK_GL_ERROR();
}

void DX12Backend::do_glUniform3fv(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform3fv(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 2]._int),
        batch._params[paramOffset + 1]._uint,
        (const GLfloat*)batch.readData(batch._params[paramOffset + 0]._uint));

    (void)CHECK_GL_ERROR();
}

void DX12Backend::do_glUniform4fv(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();

    GLint location = GET_UNIFORM_LOCATION(batch._params[paramOffset + 2]._int);
    GLsizei count = batch._params[paramOffset + 1]._uint;
    const GLfloat* value = (const GLfloat*)batch.readData(batch._params[paramOffset + 0]._uint);
    glUniform4fv(location, count, value);

    (void)CHECK_GL_ERROR();
}

void DX12Backend::do_glUniform4iv(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();
    glUniform4iv(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 2]._int),
        batch._params[paramOffset + 1]._uint,
        (const GLint*)batch.readData(batch._params[paramOffset + 0]._uint));

    (void)CHECK_GL_ERROR();
}

void DX12Backend::do_glUniformMatrix3fv(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();

    glUniformMatrix3fv(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 3]._int),
        batch._params[paramOffset + 2]._uint,
        batch._params[paramOffset + 1]._uint,
        (const GLfloat*)batch.readData(batch._params[paramOffset + 0]._uint));
    (void)CHECK_GL_ERROR();
}

void DX12Backend::do_glUniformMatrix4fv(const Batch& batch, size_t paramOffset) {
    if (_pipeline._program == 0) {
        // We should call updatePipeline() to bind the program but we are not doing that
        // because these uniform setters are deprecated and we don;t want to create side effect
        return;
    }
    updatePipeline();

    glUniformMatrix4fv(
        GET_UNIFORM_LOCATION(batch._params[paramOffset + 3]._int),
        batch._params[paramOffset + 2]._uint,
        batch._params[paramOffset + 1]._uint,
        (const GLfloat*)batch.readData(batch._params[paramOffset + 0]._uint));
    (void)CHECK_GL_ERROR();
}

void DX12Backend::do_glColor4f(const Batch& batch, size_t paramOffset) {

    glm::vec4 newColor(
        batch._params[paramOffset + 3]._float,
        batch._params[paramOffset + 2]._float,
        batch._params[paramOffset + 1]._float,
        batch._params[paramOffset + 0]._float);

    if (_input._colorAttribute != newColor) {
        _input._colorAttribute = newColor;
        glVertexAttrib4fv(gpu::Stream::COLOR, &_input._colorAttribute.r);
    }
    (void)CHECK_GL_ERROR();
}

void DX12Backend::releaseBuffer(GLuint id, Size size) const {
    Lock lock(_trashMutex);
    _buffersTrash.push_back({ id, size });
}

void DX12Backend::releaseExternalTexture(GLuint id, const Texture::ExternalRecycler& recycler) const {
    Lock lock(_trashMutex);
    _externalTexturesTrash.push_back({ id, recycler });
}

void DX12Backend::releaseTexture(GLuint id, Size size) const {
    Lock lock(_trashMutex);
    _texturesTrash.push_back({ id, size });
}

void DX12Backend::releaseFramebuffer(GLuint id) const {
    Lock lock(_trashMutex);
    _framebuffersTrash.push_back(id);
}

void DX12Backend::releaseShader(GLuint id) const {
    Lock lock(_trashMutex);
    _shadersTrash.push_back(id);
}

void DX12Backend::releaseProgram(GLuint id) const {
    Lock lock(_trashMutex);
    _programsTrash.push_back(id);
}

void DX12Backend::releaseQuery(GLuint id) const {
    Lock lock(_trashMutex);
    _queriesTrash.push_back(id);
}

void DX12Backend::queueLambda(const std::function<void()> lambda) const {
    Lock lock(_trashMutex);
    _lambdaQueue.push_back(lambda);
}

void DX12Backend::recycle() const {
    PROFILE_RANGE(render_gpu_dx, __FUNCTION__)
    {
        std::list<std::function<void()>> lamdbasTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_lambdaQueue, lamdbasTrash);
        }
        for (auto lambda : lamdbasTrash) {
            lambda();
        }
    }

    {
        std::vector<GLuint> ids;
        std::list<std::pair<GLuint, Size>> buffersTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_buffersTrash, buffersTrash);
        }
        ids.reserve(buffersTrash.size());
        for (auto pair : buffersTrash) {
            ids.push_back(pair.first);
        }
        if (!ids.empty()) {
            glDeleteBuffers((GLsizei)ids.size(), ids.data());
        }
    }

    {
        std::vector<GLuint> ids;
        std::list<GLuint> framebuffersTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_framebuffersTrash, framebuffersTrash);
        }
        ids.reserve(framebuffersTrash.size());
        for (auto id : framebuffersTrash) {
            ids.push_back(id);
        }
        if (!ids.empty()) {
            glDeleteFramebuffers((GLsizei)ids.size(), ids.data());
        }
    }

    {
        std::vector<GLuint> ids;
        std::list<std::pair<GLuint, Size>> texturesTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_texturesTrash, texturesTrash);
        }
        ids.reserve(texturesTrash.size());
        for (auto pair : texturesTrash) {
            ids.push_back(pair.first);
        }
        if (!ids.empty()) {
            glDeleteTextures((GLsizei)ids.size(), ids.data());
        }
    }

    {
        std::list<std::pair<GLuint, Texture::ExternalRecycler>> externalTexturesTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_externalTexturesTrash, externalTexturesTrash);
        }
        if (!externalTexturesTrash.empty()) {
            std::vector<GLsync> fences;  
            fences.resize(externalTexturesTrash.size());
            for (size_t i = 0; i < externalTexturesTrash.size(); ++i) {
                fences[i] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            }
            // External texture fences will be read in another thread/context, so we need a flush
            glFlush();
            size_t index = 0;
            for (auto pair : externalTexturesTrash) {
                auto fence = fences[index++];
                pair.second(pair.first, fence);
            }
        }
    }

    {
        std::list<GLuint> programsTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_programsTrash, programsTrash);
        }
        for (auto id : programsTrash) {
            glDeleteProgram(id);
        }
    }

    {
        std::list<GLuint> shadersTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_shadersTrash, shadersTrash);
        }
        for (auto id : shadersTrash) {
            glDeleteShader(id);
        }
    }

    {
        std::vector<GLuint> ids;
        std::list<GLuint> queriesTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_queriesTrash, queriesTrash);
        }
        ids.reserve(queriesTrash.size());
        for (auto id : queriesTrash) {
            ids.push_back(id);
        }
        if (!ids.empty()) {
            glDeleteQueries((GLsizei)ids.size(), ids.data());
        }
    }

    GLVariableAllocationSupport::manageMemory();
    GLVariableAllocationSupport::_frameTexturesCreated = 0;

}

void DX12Backend::setCameraCorrection(const Mat4& correction) {
    _transform._correction.correction = correction;
    _transform._correction.correctionInverse = glm::inverse(correction);
    _pipeline._cameraCorrectionBuffer._buffer->setSubData(0, _transform._correction);
    _pipeline._cameraCorrectionBuffer._buffer->flush();
}

//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL45Backend.h"

#include <mutex>
#include <queue>
#include <list>
#include <functional>
#include <glm/gtc/type_ptr.hpp>

Q_LOGGING_CATEGORY(gpugl45logging, "hifi.gpu.gl45")

using namespace gpu;
using namespace gpu::gl45;

const std::string GL45Backend::GL45_VERSION { "GL45" };

void GL45Backend::recycle() const {
    Parent::recycle();
}

void GL45Backend::do_draw(const Batch& batch, size_t paramOffset) {
    Primitive primitiveType = (Primitive)batch._params[paramOffset + 2]._uint;
    GLenum mode = gl::PRIMITIVE_TO_GL[primitiveType];
    uint32 numVertices = batch._params[paramOffset + 1]._uint;
    uint32 startVertex = batch._params[paramOffset + 0]._uint;

    if (isStereo()) {
#ifdef GPU_STEREO_DRAWCALL_INSTANCED
        glDrawArraysInstanced(mode, startVertex, numVertices, 2);
#else
        setupStereoSide(0);
        glDrawArrays(mode, startVertex, numVertices);
        setupStereoSide(1);
        glDrawArrays(mode, startVertex, numVertices);
#endif

        _stats._DSNumTriangles += 2 * numVertices / 3;
        _stats._DSNumDrawcalls += 2;

    } else {
        glDrawArrays(mode, startVertex, numVertices);
        _stats._DSNumTriangles += numVertices / 3;
        _stats._DSNumDrawcalls++;
    }
    _stats._DSNumAPIDrawcalls++;

    (void) CHECK_GL_ERROR();
}

void GL45Backend::do_drawIndexed(const Batch& batch, size_t paramOffset) {
    Primitive primitiveType = (Primitive)batch._params[paramOffset + 2]._uint;
    GLenum mode = gl::PRIMITIVE_TO_GL[primitiveType];
    uint32 numIndices = batch._params[paramOffset + 1]._uint;
    uint32 startIndex = batch._params[paramOffset + 0]._uint;

    GLenum glType = gl::ELEMENT_TYPE_TO_GL[_input._indexBufferType];

    auto typeByteSize = TYPE_SIZE[_input._indexBufferType];
    GLvoid* indexBufferByteOffset = reinterpret_cast<GLvoid*>(startIndex * typeByteSize + _input._indexBufferOffset);

    if (isStereo()) {
#ifdef GPU_STEREO_DRAWCALL_INSTANCED
        glDrawElementsInstanced(mode, numIndices, glType, indexBufferByteOffset, 2);
#else
        setupStereoSide(0);
        glDrawElements(mode, numIndices, glType, indexBufferByteOffset);
        setupStereoSide(1);
        glDrawElements(mode, numIndices, glType, indexBufferByteOffset);
#endif
        _stats._DSNumTriangles += 2 * numIndices / 3;
        _stats._DSNumDrawcalls += 2;
    } else {
        glDrawElements(mode, numIndices, glType, indexBufferByteOffset);
        _stats._DSNumTriangles += numIndices / 3;
        _stats._DSNumDrawcalls++;
    }
    _stats._DSNumAPIDrawcalls++;

    (void) CHECK_GL_ERROR();
}

void GL45Backend::do_drawInstanced(const Batch& batch, size_t paramOffset) {
    GLint numInstances = batch._params[paramOffset + 4]._uint;
    Primitive primitiveType = (Primitive)batch._params[paramOffset + 3]._uint;
    GLenum mode = gl::PRIMITIVE_TO_GL[primitiveType];
    uint32 numVertices = batch._params[paramOffset + 2]._uint;
    uint32 startVertex = batch._params[paramOffset + 1]._uint;


    if (isStereo()) {
        GLint trueNumInstances = 2 * numInstances;

#ifdef GPU_STEREO_DRAWCALL_INSTANCED
        glDrawArraysInstanced(mode, startVertex, numVertices, trueNumInstances);
#else
        setupStereoSide(0);
        glDrawArraysInstanced(mode, startVertex, numVertices, numInstances);
        setupStereoSide(1);
        glDrawArraysInstanced(mode, startVertex, numVertices, numInstances);
#endif

        _stats._DSNumTriangles += (trueNumInstances * numVertices) / 3;
        _stats._DSNumDrawcalls += trueNumInstances;
    } else {
        glDrawArraysInstanced(mode, startVertex, numVertices, numInstances);
        _stats._DSNumTriangles += (numInstances * numVertices) / 3;
        _stats._DSNumDrawcalls += numInstances;
    }
    _stats._DSNumAPIDrawcalls++;

    (void) CHECK_GL_ERROR();
}

void GL45Backend::do_drawIndexedInstanced(const Batch& batch, size_t paramOffset) {
    GLint numInstances = batch._params[paramOffset + 4]._uint;
    GLenum mode = gl::PRIMITIVE_TO_GL[(Primitive)batch._params[paramOffset + 3]._uint];
    uint32 numIndices = batch._params[paramOffset + 2]._uint;
    uint32 startIndex = batch._params[paramOffset + 1]._uint;
    uint32 startInstance = batch._params[paramOffset + 0]._uint;
    GLenum glType = gl::ELEMENT_TYPE_TO_GL[_input._indexBufferType];
    auto typeByteSize = TYPE_SIZE[_input._indexBufferType];
    GLvoid* indexBufferByteOffset = reinterpret_cast<GLvoid*>(startIndex * typeByteSize + _input._indexBufferOffset);

    if (isStereo()) {
        GLint trueNumInstances = 2 * numInstances;

#ifdef GPU_STEREO_DRAWCALL_INSTANCED
        glDrawElementsInstancedBaseVertexBaseInstance(mode, numIndices, glType, indexBufferByteOffset, trueNumInstances, 0, startInstance);
#else
        setupStereoSide(0);
        glDrawElementsInstancedBaseVertexBaseInstance(mode, numIndices, glType, indexBufferByteOffset, numInstances, 0, startInstance);
        setupStereoSide(1);
        glDrawElementsInstancedBaseVertexBaseInstance(mode, numIndices, glType, indexBufferByteOffset, numInstances, 0, startInstance);
#endif
        _stats._DSNumTriangles += (trueNumInstances * numIndices) / 3;
        _stats._DSNumDrawcalls += trueNumInstances;
    } else {
        glDrawElementsInstancedBaseVertexBaseInstance(mode, numIndices, glType, indexBufferByteOffset, numInstances, 0, startInstance);
        _stats._DSNumTriangles += (numInstances * numIndices) / 3;
        _stats._DSNumDrawcalls += numInstances;
    }

    _stats._DSNumAPIDrawcalls++;

    (void)CHECK_GL_ERROR();
}

void GL45Backend::do_multiDrawIndirect(const Batch& batch, size_t paramOffset) {
    uint commandCount = batch._params[paramOffset + 0]._uint;
    GLenum mode = gl::PRIMITIVE_TO_GL[(Primitive)batch._params[paramOffset + 1]._uint];
    glMultiDrawArraysIndirect(mode, reinterpret_cast<GLvoid*>(_input._indirectBufferOffset), commandCount, (GLsizei)_input._indirectBufferStride);
    _stats._DSNumDrawcalls += commandCount;
    _stats._DSNumAPIDrawcalls++;
    (void)CHECK_GL_ERROR();
}

void GL45Backend::do_multiDrawIndexedIndirect(const Batch& batch, size_t paramOffset) {
    uint commandCount = batch._params[paramOffset + 0]._uint;
    GLenum mode = gl::PRIMITIVE_TO_GL[(Primitive)batch._params[paramOffset + 1]._uint];
    GLenum indexType = gl::ELEMENT_TYPE_TO_GL[_input._indexBufferType];
    glMultiDrawElementsIndirect(mode, indexType, reinterpret_cast<GLvoid*>(_input._indirectBufferOffset), commandCount, (GLsizei)_input._indirectBufferStride);
    _stats._DSNumDrawcalls += commandCount;
    _stats._DSNumAPIDrawcalls++;
    (void)CHECK_GL_ERROR();
}
#endif
