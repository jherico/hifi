//
//  Created by Bradley Austin Davis on 2016/08/07
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "VKBackend.h"

#include <mutex>
#include <queue>
#include <list>
#include <functional>
#include <glm/gtc/type_ptr.hpp>

#include <QtCore/QProcessEnvironment>

#include <vk/Helpers.h>
#include <vk/Version.h>
#include <vk/Pipelines.h>

#include "VKForward.h"
#include "VKShared.h"

using namespace gpu;
using namespace gpu::vulkan;

static VKBackend* INSTANCE{ nullptr };
static const char* VK_BACKEND_PROPERTY_NAME = "com.highfidelity.vk.backend";
static bool enableDebugMarkers = false;

BackendPointer VKBackend::createBackend() {
    // FIXME provide a mechanism to override the backend for testing
    // Where the gpuContext is initialized and where the TRUE Backend is created and assigned
    std::shared_ptr<VKBackend> result = std::make_shared<VKBackend>();
    INSTANCE = result.get();
    void* voidInstance = &(*result);
    qApp->setProperty(VK_BACKEND_PROPERTY_NAME, QVariant::fromValue(voidInstance));
    return result;
}

VKBackend& getBackend() {
    if (!INSTANCE) {
        INSTANCE = static_cast<VKBackend*>(qApp->property(VK_BACKEND_PROPERTY_NAME).value<void*>());
    }
    return *INSTANCE;
}

void VKBackend::init() {
}

VKBackend::VKBackend() {
    _context.create();
    {
        vk::PipelineCacheCreateInfo createInfo;
        std::vector<uint8_t> pipelineCacheData;
        if (vks::util::loadPipelineCacheData(pipelineCacheData) && !pipelineCacheData.empty()) {
            createInfo.pInitialData = pipelineCacheData.data();
            createInfo.initialDataSize = pipelineCacheData.size();
        }
        _pipelineCache = _device.createPipelineCache(createInfo);
    }

    // Get the graphics queue
    qCDebug(gpu_vk_logging) << "VK Version: " << vks::Version(_physicalDeviceProperties.apiVersion).toString().c_str();
    qCDebug(gpu_vk_logging) << "VK Driver Version: "
                            << vks::Version(_physicalDeviceProperties.driverVersion).toString().c_str();
    qCDebug(gpu_vk_logging) << "VK Vendor ID: " << _physicalDeviceProperties.vendorID;
    qCDebug(gpu_vk_logging) << "VK Device ID: " << _physicalDeviceProperties.deviceID;
    qCDebug(gpu_vk_logging) << "VK Device Name: " << _physicalDeviceProperties.deviceName;
    qCDebug(gpu_vk_logging) << "VK Device Type: " << to_string(_physicalDeviceProperties.deviceType).c_str();
}

VKBackend::~VKBackend() {
    // FIXME queue up all the trash calls
    _graphicsQueue.waitIdle();
    _transferQueue.waitIdle();
    _device.waitIdle();

    {
        std::vector<uint8_t> pipelineCacheData;
        pipelineCacheData = _device.getPipelineCacheData(_pipelineCache);
        if (!pipelineCacheData.empty()) {
            vks::util::savePipelineCacheData(pipelineCacheData);
        }
    }

    _context.destroyContext();
}

const std::string& VKBackend::getVersion() const {
    static const std::string VERSION{ "VK1.1" };
    return VERSION;
}

bool VKBackend::isTextureManagementSparseEnabled() const {
    return false;
    //return (_context.deviceFeatures.sparseResidencyImage2D = VK_TRUE);
}

bool VKBackend::supportedTextureFormat(const gpu::Element& format) const {
    switch (format.getSemantic()) {
        case COMPRESSED_BC1_SRGB:
        case COMPRESSED_BC1_SRGBA:
        case COMPRESSED_BC3_SRGBA:
        case COMPRESSED_BC4_RED:
        case COMPRESSED_BC5_XY:
        case COMPRESSED_BC6_RGB:
        case COMPRESSED_BC7_SRGBA:
            return _context.deviceFeatures.textureCompressionBC == VK_TRUE;

        case COMPRESSED_ETC2_RGB:
        case COMPRESSED_ETC2_SRGB:
        case COMPRESSED_ETC2_RGB_PUNCHTHROUGH_ALPHA:
        case COMPRESSED_ETC2_SRGB_PUNCHTHROUGH_ALPHA:
        case COMPRESSED_ETC2_RGBA:
        case COMPRESSED_ETC2_SRGBA:
        case COMPRESSED_EAC_RED:
        case COMPRESSED_EAC_RED_SIGNED:
        case COMPRESSED_EAC_XY:
        case COMPRESSED_EAC_XY_SIGNED:
            return _context.deviceFeatures.textureCompressionETC2 == VK_TRUE;

            //case COMPRESSED_ASTC_RGBA_10x10:
            //case COMPRESSED_ASTC_RGBA_10x5:
            //case COMPRESSED_ASTC_RGBA_10x6:
            //case COMPRESSED_ASTC_RGBA_10x8:
            //case COMPRESSED_ASTC_RGBA_12x10:
            //case COMPRESSED_ASTC_RGBA_12x12:
            //case COMPRESSED_ASTC_RGBA_4x4:
            //case COMPRESSED_ASTC_RGBA_5x4:
            //case COMPRESSED_ASTC_RGBA_5x5:
            //case COMPRESSED_ASTC_RGBA_6x5:
            //case COMPRESSED_ASTC_RGBA_6x6:
            //case COMPRESSED_ASTC_RGBA_8x5:
            //case COMPRESSED_ASTC_RGBA_8x6:
            //case COMPRESSED_ASTC_RGBA_8x8:
            //case COMPRESSED_ASTC_SRGB8_ALPHA8_10x10:
            //case COMPRESSED_ASTC_SRGB8_ALPHA8_10x5:
            //case COMPRESSED_ASTC_SRGB8_ALPHA8_10x6:
            //case COMPRESSED_ASTC_SRGB8_ALPHA8_10x8:
            //case COMPRESSED_ASTC_SRGB8_ALPHA8_12x10:
            //case COMPRESSED_ASTC_SRGB8_ALPHA8_12x12:
            //case COMPRESSED_ASTC_SRGB8_ALPHA8_4x4:
            //case COMPRESSED_ASTC_SRGB8_ALPHA8_5x4:
            //case COMPRESSED_ASTC_SRGB8_ALPHA8_5x5:
            //case COMPRESSED_ASTC_SRGB8_ALPHA8_6x5:
            //case COMPRESSED_ASTC_SRGB8_ALPHA8_6x6:
            //case COMPRESSED_ASTC_SRGB8_ALPHA8_8x5:
            //case COMPRESSED_ASTC_SRGB8_ALPHA8_8x6:
            //case COMPRESSED_ASTC_SRGB8_ALPHA8_8x8:
            // return _context.deviceFeatures.textureCompressionASTC_LDR == VK_TRUE;

        default:
            break;
    }
    return true;
}

struct PipelineBuilder {
    PipelineBuilder() {}
    ~PipelineBuilder() {}

    bool dirty{ true };

    void updateVertexFormat(const gpu::Stream::Format& format) {}
};

struct Cache {
    struct Pipeline {
        gpu::PipelineReference pipeline{ GPU_REFERENCE_INIT_VALUE };
        gpu::FormatReference format{ GPU_REFERENCE_INIT_VALUE };
        bool dirty = false;


        void setPipeline(const gpu::PipelinePointer& pipeline) {
            if (!gpu::compare(this->pipeline, pipeline)) {
                gpu::assign(this->pipeline, pipeline);
                dirty = true;
            }
        }

        void setVertexFormat(const gpu::Stream::FormatPointer& format) {
            if (!gpu::compare(this->format, format)) {
                gpu::assign(this->format, format);
                dirty = true;
            }
        }

    } pipelineState;

    struct Renderpass {
        gpu::FramebufferReference framebuffer{ GPU_REFERENCE_INIT_VALUE };
    } renderpassState;


    vk::PipelineLayout getPipelineLayout() {
        return nullptr;
    }

    vk::RenderPass getRenderPass() {
        return nullptr;
    }

    static vk::StencilOpState getStencilOp(const gpu::State::StencilTest& stencil) { 
        vk::StencilOpState result;
        result.compareOp = (vk::CompareOp)stencil.getFunction();
        result.passOp = (vk::StencilOp)stencil.getPassOp();
        result.failOp = (vk::StencilOp)stencil.getFailOp();
        result.depthFailOp = (vk::StencilOp)stencil.getDepthFailOp();
        result.reference = stencil.getReference();
        result.compareMask = stencil.getReadMask();
        result.writeMask = 0xFF;
        return result;
    }


    vk::Pipeline getPipeline(const vk::Device& device) {
        const gpu::Pipeline& pipeline = *gpu::acquire(pipelineState.pipeline);
        const gpu::State& state = *pipeline.getState();
        const gpu::State::Data& stateData = state.getValues();
        
        vks::pipelines::GraphicsPipelineBuilder builder{ device, getPipelineLayout(), getRenderPass() };

        // Input assembly
        {
            auto& ia = builder.inputAssemblyState;
            // ia.primitiveRestartEnable = ???
            // ia.topology = vk::PrimitiveTopology::eTriangleList; ???
        }

        // Rasterization state
        { 
            auto& ra = builder.rasterizationState;
            ra.cullMode = (vk::CullModeFlagBits)stateData.cullMode;
            //Bool32 ra.depthBiasEnable;
            //float ra.depthBiasConstantFactor;
            //float ra.depthBiasClamp;
            //float ra.depthBiasSlopeFactor;
            ra.depthClampEnable = stateData.depthClampEnable ? VK_TRUE : VK_FALSE;
            ra.frontFace = stateData.frontFaceClockwise ? vk::FrontFace::eClockwise : vk::FrontFace::eCounterClockwise;
            // ra.lineWidth 
            ra.polygonMode = (vk::PolygonMode)(2 - stateData.fillMode);
        }

        // Color blending
        { 
            auto& cb = builder.colorBlendState;
            //cb.blendConstants
        }

        // Depth/Stencil
        { 
            auto& ds = builder.depthStencilState;
            ds.depthTestEnable = stateData.depthTest.isEnabled() ? VK_TRUE : VK_FALSE;
            ds.depthWriteEnable = stateData.depthTest.getWriteMask() != 0 ? VK_TRUE : VK_FALSE;
            ds.depthCompareOp = (vk::CompareOp)stateData.depthTest.getFunction();
            ds.front = getStencilOp(stateData.stencilTestFront);
            ds.back = getStencilOp(stateData.stencilTestBack);
        }


        // Vertex input
        { 
            const gpu::Stream::Format& format = *gpu::acquire(pipelineState.format);
            vk::VertexInputBindingDescription vb;
            vk::VertexInputAttributeDescription via;
        }

        return nullptr;
    }



};

Cache _cache;

void VKBackend::executeFrame(const FramePointer& frame) {
    //_stereo._skybox = batch.isSkyboxEnabled();
    //// Allow the batch to override the rendering stereo settings
    //// for things like full framebuffer copy operations (deferred lighting passes)
    //bool savedStereo = _stereo._enable;
    //if (!batch.isStereoEnabled()) {
    //    _stereo._enable = false;
    //}

    {
        PROFILE_RANGE(gpu_vk_detail, "Preprocess");
        for (const auto& batchPtr : frame->batches) {
            const auto& batch = *batchPtr;
            const auto& commands = batch.getCommands();
            const auto& offsets = batch.getCommandOffsets();
            const auto numCommands = commands.size();
            const auto* command = commands.data();
            const auto* offset = offsets.data();
            for (auto commandIndex = 0; commandIndex < numCommands; ++commandIndex, ++command, ++offset) {
                const auto& paramOffset = *offset;
                // How to resolve renderpass
                switch (*command) {
                    case Batch::COMMAND_draw:
                    case Batch::COMMAND_drawIndexed:
                    case Batch::COMMAND_drawInstanced:
                    case Batch::COMMAND_drawIndexedInstanced:
                    case Batch::COMMAND_multiDrawIndirect:
                    case Batch::COMMAND_multiDrawIndexedIndirect:
                        // resolve layout
                        // resolve pipeline
                        // resolve descriptor set(s)
                        _cache.getPipeline(_context.device);
                        break;

                    case Batch::COMMAND_setPipeline: {
                        const auto& pipeline = batch._pipelines.get(batch._params[paramOffset]._uint);
                        _cache.pipelineState.setPipeline(pipeline);
                    } break;

                    case Batch::COMMAND_setInputFormat: {
                        const auto& format = batch._streamFormats.get(batch._params[paramOffset]._uint);
                        _cache.pipelineState.setVertexFormat(format);
                    } break;

                    case Batch::COMMAND_setInputBuffer:
                    case Batch::COMMAND_setIndexBuffer:
                    case Batch::COMMAND_setIndirectBuffer:

                    case Batch::COMMAND_setModelTransform:
                    case Batch::COMMAND_setViewTransform:
                    case Batch::COMMAND_setProjectionTransform:
                    case Batch::COMMAND_setProjectionJitter:
                    case Batch::COMMAND_setViewportTransform:
                    case Batch::COMMAND_setDepthRangeTransform:

                    case Batch::COMMAND_setStateBlendFactor:
                    case Batch::COMMAND_setStateScissorRect:

                    case Batch::COMMAND_setUniformBuffer:
                    case Batch::COMMAND_setResourceBuffer:
                    case Batch::COMMAND_setResourceTexture:
                    case Batch::COMMAND_setResourceTextureTable:
                    case Batch::COMMAND_setResourceFramebufferSwapChainTexture:

                    case Batch::COMMAND_setFramebuffer:
                    case Batch::COMMAND_setFramebufferSwapChain:
                    case Batch::COMMAND_clearFramebuffer:
                    case Batch::COMMAND_blit:
                    case Batch::COMMAND_generateTextureMips:

                    case Batch::COMMAND_advance:

                    case Batch::COMMAND_beginQuery:
                    case Batch::COMMAND_endQuery:
                    case Batch::COMMAND_getQuery:

                    case Batch::COMMAND_resetStages:

                    case Batch::COMMAND_disableContextViewCorrection:
                    case Batch::COMMAND_restoreContextViewCorrection:

                    case Batch::COMMAND_disableContextStereo:
                    case Batch::COMMAND_restoreContextStereo:

                    case Batch::COMMAND_runLambda:

                    case Batch::COMMAND_startNamedCall:
                    case Batch::COMMAND_stopNamedCall:
                    case Batch::COMMAND_glColor4f:

                    case Batch::COMMAND_pushProfileRange:
                    case Batch::COMMAND_popProfileRange:
                        break;
                }
                // loop through commands
                // did the framebuffer setup change?  (new renderpass)
                // did the descriptor setup change?
                // did the pipeline / vertex format change?
                // derive pipeline layout
                // generate pipeline
                // find unique descriptor targets
                // do we need to transfer data to the GPU?
            }
        }
    }

    // loop through commands

    //

    //{
    //    PROFILE_RANGE(gpu_vk_detail, _stereo._enable ? "Render Stereo" : "Render");
    //    renderPassDraw(batch);
    //}

    // Restore the saved stereo state for the next batch
    // _stereo._enable = savedStereo;
}

#if 0
void VKBackend::do_resetStages(const Batch& batch, size_t paramOffset) {
}

void VKBackend::do_runLambda(const Batch& batch, size_t paramOffset) {
    std::function<void()> f = batch._lambdas.get(batch._params[paramOffset]._uint);
    f();
}

void VKBackend::do_startNamedCall(const Batch& batch, size_t paramOffset) {
    batch._currentNamedCall = batch._names.get(batch._params[paramOffset]._uint);
}

void VKBackend::do_stopNamedCall(const Batch& batch, size_t paramOffset) {
    batch._currentNamedCall.clear();
}

void VKBackend::do_pushProfileRange(const Batch& batch, size_t paramOffset) {
    const auto& name = batch._profileRanges.get(batch._params[paramOffset]._uint);
    ::vks::debug::marker::beginRegion(vk::CommandBuffer{}, name, glm::vec4{ 1.0 });
}

void VKBackend::do_popProfileRange(const Batch& batch, size_t paramOffset) {
    ::vks::debug::marker::endRegion(vk::CommandBuffer{});
}

void VKBackend::setCameraCorrection(const Mat4& correction) {
}

void VKBackend::renderPassTransfer(Batch& batch) {
    const size_t numCommands = batch.getCommands().size();
    const Batch::Commands::value_type* command = batch.getCommands().data();
    const Batch::CommandOffsets::value_type* offset = batch.getCommandOffsets().data();

    _inRenderTransferPass = true;
    { // Sync all the buffers
        PROFILE_RANGE("syncGPUBuffer");

        for (auto& cached : batch._buffers._items) {
            if (cached._data) {
                syncGPUObject(*cached._data);
            }
        }
    }

    { // Sync all the buffers
        PROFILE_RANGE("syncCPUTransform");
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
        PROFILE_RANGE("syncGPUTransform");
        transferTransformState(batch);
    }

    _inRenderTransferPass = false;
}

void VKBackend::renderPassDraw(const Batch& batch) {
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

void VKBackend::setupStereoSide(int side) {
    ivec4 vp = _transform._viewport;
    vp.z /= 2;
    glViewport(vp.x + side * vp.z, vp.y, vp.z, vp.w);

    _transform.bindCurrentCamera(side);
}


void VKBackend::do_setFramebuffer(const Batch& batch, size_t paramOffset) {
    auto framebuffer = batch._framebuffers.get(batch._params[paramOffset]._uint);
    if (_output._framebuffer != framebuffer) {
        auto newFBO = getFramebufferID(framebuffer);
        if (_output._drawFBO != newFBO) {
            _output._drawFBO = newFBO;
            glBindFramebuffer(VK_DRAW_FRAMEBUFFER, newFBO);
        }
        _output._framebuffer = framebuffer;
    }
}

void VKBackend::do_clearFramebuffer(const Batch& batch, size_t paramOffset) {
    if (_stereo._enable && !_pipeline._stateCache.scissorEnable) {
        qWarning("Clear without scissor in stereo mode");
    }

    uint32 masks = batch._params[paramOffset + 7]._uint;
    Vec4 color;
    color.x = batch._params[paramOffset + 6]._float;
    color.y = batch._params[paramOffset + 5]._float;
    color.z = batch._params[paramOffset + 4]._float;
    color.w = batch._params[paramOffset + 3]._float;
    float depth = batch._params[paramOffset + 2]._float;
    int stencil = batch._params[paramOffset + 1]._int;
    int useScissor = batch._params[paramOffset + 0]._int;

    VKuint glmask = 0;
    if (masks & Framebuffer::BUFFER_STENCIL) {
        glClearStencil(stencil);
        glmask |= VK_STENCIL_BUFFER_BIT;
        // TODO: we will probably need to also check the write mask of stencil like we do
        // for depth buffer, but as would say a famous Fez owner "We'll cross that bridge when we come to it"
    }

    bool restoreDepthMask = false;
    if (masks & Framebuffer::BUFFER_DEPTH) {
        glClearDepth(depth);
        glmask |= VK_DEPTH_BUFFER_BIT;

        bool cacheDepthMask = _pipeline._stateCache.depthTest.getWriteMask();
        if (!cacheDepthMask) {
            restoreDepthMask = true;
            glDepthMask(VK_TRUE);
        }
    }

    std::vector<VKenum> drawBuffers;
    if (masks & Framebuffer::BUFFER_COLORS) {
        if (_output._framebuffer) {
            for (unsigned int i = 0; i < Framebuffer::MAX_NUM_RENDER_BUFFERS; i++) {
                if (masks & (1 << i)) {
                    drawBuffers.push_back(VK_COLOR_ATTACHMENT0 + i);
                }
            }

            if (!drawBuffers.empty()) {
                glDrawBuffers((VKsizei)drawBuffers.size(), drawBuffers.data());
                glClearColor(color.x, color.y, color.z, color.w);
                glmask |= VK_COLOR_BUFFER_BIT;

                (void)CHECK_VK_ERROR();
            }
        } else {
            glClearColor(color.x, color.y, color.z, color.w);
            glmask |= VK_COLOR_BUFFER_BIT;
        }

        // Force the color mask cache to WRITE_ALL if not the case
        do_setStateColorWriteMask(State::ColorMask::WRITE_ALL);
    }

    // Apply scissor if needed and if not already on
    bool doEnableScissor = (useScissor && (!_pipeline._stateCache.scissorEnable));
    if (doEnableScissor) {
        glEnable(VK_SCISSOR_TEST);
    }

    // Clear!
    glClear(glmask);

    // Restore scissor if needed
    if (doEnableScissor) {
        glDisable(VK_SCISSOR_TEST);
    }

    // Restore write mask meaning turn back off
    if (restoreDepthMask) {
        glDepthMask(VK_FALSE);
    }

    // Restore the color draw buffers only if a frmaebuffer is bound
    if (_output._framebuffer && !drawBuffers.empty()) {
        auto glFramebuffer = syncGPUObject(*_output._framebuffer);
        if (glFramebuffer) {
            glDrawBuffers((VKsizei)glFramebuffer->_colorBuffers.size(), glFramebuffer->_colorBuffers.data());
        }
    }

    (void)CHECK_VK_ERROR();
}

void VKBackend::downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage) {
}

void VKBackend::do_setInputFormat(const Batch& batch, size_t paramOffset) {
    Stream::FormatPointer format = batch._streamFormats.get(batch._params[paramOffset]._uint);

    if (format != _input._format) {
        _input._format = format;
        _input._invalidFormat = true;
    }
}

void VKBackend::do_setInputBuffer(const Batch& batch, size_t paramOffset) {
    Offset stride = batch._params[paramOffset + 0]._uint;
    Offset offset = batch._params[paramOffset + 1]._uint;
    BufferPointer buffer = batch._buffers.get(batch._params[paramOffset + 2]._uint);
    uint32 channel = batch._params[paramOffset + 3]._uint;

    if (channel < getNumInputBuffers()) {
        bool isModified = false;
        if (_input._buffers[channel] != buffer) {
            _input._buffers[channel] = buffer;

            VKuint vbo = 0;
            if (buffer) {
                vbo = getBufferID((*buffer));
            }
            _input._bufferVBOs[channel] = vbo;

            isModified = true;
        }

        if (_input._bufferOffsets[channel] != offset) {
            _input._bufferOffsets[channel] = offset;
            isModified = true;
        }

        if (_input._bufferStrides[channel] != stride) {
            _input._bufferStrides[channel] = stride;
            isModified = true;
        }

        if (isModified) {
            _input._invalidBuffers.set(channel);
        }
    }
}

void VKBackend::do_setIndexBuffer(const Batch& batch, size_t paramOffset) {
    _input._indexBufferType = (Type)batch._params[paramOffset + 2]._uint;
    _input._indexBufferOffset = batch._params[paramOffset + 0]._uint;

    BufferPointer indexBuffer = batch._buffers.get(batch._params[paramOffset + 1]._uint);
    if (indexBuffer != _input._indexBuffer) {
        _input._indexBuffer = indexBuffer;
        if (indexBuffer) {
            glBindBuffer(VK_ELEMENT_ARRAY_BUFFER, getBufferID(*indexBuffer));
        } else {
            // FIXME do we really need this?  Is there ever a draw call where we care that the element buffer is null?
            glBindBuffer(VK_ELEMENT_ARRAY_BUFFER, 0);
        }
    }
    (void)CHECK_VK_ERROR();
}

void VKBackend::do_setIndirectBuffer(const Batch& batch, size_t paramOffset) {
    _input._indirectBufferOffset = batch._params[paramOffset + 1]._uint;
    _input._indirectBufferStride = batch._params[paramOffset + 2]._uint;

    BufferPointer buffer = batch._buffers.get(batch._params[paramOffset]._uint);
    if (buffer != _input._indirectBuffer) {
        _input._indirectBuffer = buffer;
        if (buffer) {
            glBindBuffer(VK_DRAW_INDIRECT_BUFFER, getBufferID(*buffer));
        } else {
            // FIXME do we really need this?  Is there ever a draw call where we care that the element buffer is null?
            glBindBuffer(VK_DRAW_INDIRECT_BUFFER, 0);
        }
    }

    (void)CHECK_VK_ERROR();
}

void VKBackend::do_generateTextureMips(const Batch& batch, size_t paramOffset) {
    TexturePointer resourceTexture = batch._textures.get(batch._params[paramOffset + 0]._uint);
    if (!resourceTexture) {
        return;
    }

    // DO not transfer the texture, this call is expected for rendering texture
    VKTexture* object = syncGPUObject(resourceTexture, false);
    if (!object) {
        return;
    }

    object->generateMips();
}

void VKBackend::do_beginQuery(const Batch& batch, size_t paramOffset) {
    auto query = batch._queries.get(batch._params[paramOffset]._uint);
    VKQuery* glquery = syncGPUObject(*query);
    if (glquery) {
        if (timeElapsed) {
            glBeginQuery(VK_TIME_ELAPSED, glquery->_endqo);
        } else {
            glQueryCounter(glquery->_beginqo, VK_TIMESTAMP);
        }
        (void)CHECK_VK_ERROR();
    }
}

void VKBackend::do_endQuery(const Batch& batch, size_t paramOffset) {
    auto query = batch._queries.get(batch._params[paramOffset]._uint);
    VKQuery* glquery = syncGPUObject(*query);
    if (glquery) {
        if (timeElapsed) {
            glEndQuery(VK_TIME_ELAPSED);
        } else {
            glQueryCounter(glquery->_endqo, VK_TIMESTAMP);
        }
        (void)CHECK_VK_ERROR();
    }
}

void VKBackend::do_getQuery(const Batch& batch, size_t paramOffset) {
    auto query = batch._queries.get(batch._params[paramOffset]._uint);
    VKQuery* glquery = syncGPUObject(*query);
    if (glquery) {
        glGetQueryObjectui64v(glquery->_endqo, VK_QUERY_RESULT_AVAILABLE, &glquery->_result);
        if (glquery->_result == VK_TRUE) {
            if (timeElapsed) {
                glGetQueryObjectui64v(glquery->_endqo, VK_QUERY_RESULT, &glquery->_result);
            } else {
                VKuint64 start, end;
                glGetQueryObjectui64v(glquery->_beginqo, VK_QUERY_RESULT, &start);
                glGetQueryObjectui64v(glquery->_endqo, VK_QUERY_RESULT, &end);
                glquery->_result = end - start;
            }
            query->triggerReturnHandler(glquery->_result);
        }
        (void)CHECK_VK_ERROR();
    }
}

// Transform Stage
void VKBackend::do_setModelTransform(const Batch& batch, size_t paramOffset) {
}

void VKBackend::do_setViewTransform(const Batch& batch, size_t paramOffset) {
    _transform._view = batch._transforms.get(batch._params[paramOffset]._uint);
    _transform._viewIsCamera = batch._params[paramOffset + 1]._uint != 0;
    _transform._invalidView = true;
}

void VKBackend::do_setProjectionTransform(const Batch& batch, size_t paramOffset) {
    memcpy(&_transform._projection, batch.editData(batch._params[paramOffset]._uint), sizeof(Mat4));
    _transform._invalidProj = true;
}

void VKBackend::do_setViewportTransform(const Batch& batch, size_t paramOffset) {
    memcpy(&_transform._viewport, batch.editData(batch._params[paramOffset]._uint), sizeof(Vec4i));

    if (!_inRenderTransferPass && !isStereo()) {
        ivec4& vp = _transform._viewport;
        glViewport(vp.x, vp.y, vp.z, vp.w);
    }

    // The Viewport is tagged invalid because the CameraTransformUBO is not up to date and will need update on next drawcall
    _transform._invalidViewport = true;
}

void VKBackend::do_setDepthRangeTransform(const Batch& batch, size_t paramOffset) {
    Vec2 depthRange(batch._params[paramOffset + 1]._float, batch._params[paramOffset + 0]._float);

    if ((depthRange.x != _transform._depthRange.x) || (depthRange.y != _transform._depthRange.y)) {
        _transform._depthRange = depthRange;

        glDepthRangef(depthRange.x, depthRange.y);
    }
}

void VKBackend::do_setStateScissorRect(const Batch& batch, size_t paramOffset) {
    Vec4i rect;
    memcpy(&rect, batch.editData(batch._params[paramOffset]._uint), sizeof(Vec4i));

    if (_stereo._enable) {
        rect.z /= 2;
        if (_stereo._pass) {
            rect.x += rect.z;
        }
    }
    glScissor(rect.x, rect.y, rect.z, rect.w);
    (void)CHECK_VK_ERROR();
}

void VKBackend::do_setPipeline(const Batch& batch, size_t paramOffset) {
    PipelinePointer pipeline = batch._pipelines.get(batch._params[paramOffset + 0]._uint);

    if (_pipeline._pipeline == pipeline) {
        return;
    }

    // A true new Pipeline
    _stats._PSNumSetPipelines++;

    // null pipeline == reset
    if (!pipeline) {
        _pipeline._pipeline.reset();

        _pipeline._program = 0;
        _pipeline._cameraCorrectionLocation = -1;
        _pipeline._programShader = nullptr;
        _pipeline._invalidProgram = true;

        _pipeline._state = nullptr;
        _pipeline._invalidState = true;
    } else {
        auto pipelineObject = VKPipeline::sync(*this, *pipeline);
        if (!pipelineObject) {
            return;
        }

        // check the program cache
        // pick the program version
        VKuint glprogram = pipelineObject->_program->getProgram();

        if (_pipeline._program != glprogram) {
            _pipeline._program = glprogram;
            _pipeline._programShader = pipelineObject->_program;
            _pipeline._invalidProgram = true;
            _pipeline._cameraCorrectionLocation = pipelineObject->_cameraCorrection;
        }

        // Now for the state
        if (_pipeline._state != pipelineObject->_state) {
            _pipeline._state = pipelineObject->_state;
            _pipeline._invalidState = true;
        }

        // Remember the new pipeline
        _pipeline._pipeline = pipeline;
    }

    // THis should be done on Pipeline::update...
    if (_pipeline._invalidProgram) {
        glUseProgram(_pipeline._program);
        if (_pipeline._cameraCorrectionLocation != -1) {
            auto cameraCorrectionBuffer = syncGPUObject(*_pipeline._cameraCorrectionBuffer._buffer);
            glBindBufferRange(VK_UNIFORM_BUFFER, _pipeline._cameraCorrectionLocation, cameraCorrectionBuffer->_id, 0,
                              sizeof(CameraCorrection));
        }
        (void)CHECK_VK_ERROR();
        _pipeline._invalidProgram = false;
    }
}

void VKBackend::do_setUniformBuffer(const Batch& batch, size_t paramOffset) {
    VKuint slot = batch._params[paramOffset + 3]._uint;
    BufferPointer uniformBuffer = batch._buffers.get(batch._params[paramOffset + 2]._uint);
    VKintptr rangeStart = batch._params[paramOffset + 1]._uint;
    VKsizeiptr rangeSize = batch._params[paramOffset + 0]._uint;

    if (!uniformBuffer) {
        releaseUniformBuffer(slot);
        return;
    }

    // check cache before thinking
    if (_uniform._buffers[slot] == uniformBuffer) {
        return;
    }

    // Sync BufferObject
    auto* object = syncGPUObject(*uniformBuffer);
    if (object) {
        glBindBufferRange(VK_UNIFORM_BUFFER, slot, object->_buffer, rangeStart, rangeSize);

        _uniform._buffers[slot] = uniformBuffer;
        (void)CHECK_VK_ERROR();
    } else {
        releaseResourceTexture(slot);
        return;
    }
}

void VKBackend::do_setResourceTexture(const Batch& batch, size_t paramOffset) {
    VKuint slot = batch._params[paramOffset + 1]._uint;
    TexturePointer resourceTexture = batch._textures.get(batch._params[paramOffset + 0]._uint);

    if (!resourceTexture) {
        releaseResourceTexture(slot);
        return;
    }
    // check cache before thinking
    if (_resource._textures[slot] == resourceTexture) {
        return;
    }

    // One more True texture bound
    _stats._RSNumTextureBounded++;

    // Always make sure the VKObject is in sync
    VKTexture* object = syncGPUObject(resourceTexture);
    if (object) {
        VKuint to = object->_texture;
        VKuint target = object->_target;
        glActiveTexture(VK_TEXTURE0 + slot);
        glBindTexture(target, to);

        (void)CHECK_VK_ERROR();

        _resource._textures[slot] = resourceTexture;

        _stats._RSAmountTextureMemoryBounded += object->size();

    } else {
        releaseResourceTexture(slot);
        return;
    }
}

std::array<VKBackend::CommandCall, Batch::NUM_COMMANDS> VKBackend::_commandCalls{ {
    (&::gpu::vulkan::VKBackend::do_draw),
    (&::gpu::vulkan::VKBackend::do_drawIndexed),
    (&::gpu::vulkan::VKBackend::do_drawInstanced),
    (&::gpu::vulkan::VKBackend::do_drawIndexedInstanced),
    (&::gpu::vulkan::VKBackend::do_multiDrawIndirect),
    (&::gpu::vulkan::VKBackend::do_multiDrawIndexedIndirect),

    (&::gpu::vulkan::VKBackend::do_setInputFormat),
    (&::gpu::vulkan::VKBackend::do_setInputBuffer),
    (&::gpu::vulkan::VKBackend::do_setIndexBuffer),
    (&::gpu::vulkan::VKBackend::do_setIndirectBuffer),

    (&::gpu::vulkan::VKBackend::do_setModelTransform),
    (&::gpu::vulkan::VKBackend::do_setViewTransform),
    (&::gpu::vulkan::VKBackend::do_setProjectionTransform),
    (&::gpu::vulkan::VKBackend::do_setProjectionJitter),
    (&::gpu::vulkan::VKBackend::do_setViewportTransform),
    (&::gpu::vulkan::VKBackend::do_setDepthRangeTransform),

    (&::gpu::vulkan::VKBackend::do_setPipeline),
    (&::gpu::vulkan::VKBackend::do_setStateBlendFactor),
    (&::gpu::vulkan::VKBackend::do_setStateScissorRect),

    (&::gpu::vulkan::VKBackend::do_setUniformBuffer),
    (&::gpu::vulkan::VKBackend::do_setResourceBuffer),
    (&::gpu::vulkan::VKBackend::do_setResourceTexture),
    (&::gpu::vulkan::VKBackend::do_setResourceTextureTable),
    (&::gpu::vulkan::VKBackend::do_setResourceFramebufferSwapChainTexture),

    (&::gpu::vulkan::VKBackend::do_setFramebuffer),
    (&::gpu::vulkan::VKBackend::do_setFramebufferSwapChain),
    (&::gpu::vulkan::VKBackend::do_clearFramebuffer),
    (&::gpu::vulkan::VKBackend::do_blit),
    (&::gpu::vulkan::VKBackend::do_generateTextureMips),

    (&::gpu::vulkan::VKBackend::do_advance),

    (&::gpu::vulkan::VKBackend::do_beginQuery),
    (&::gpu::vulkan::VKBackend::do_endQuery),
    (&::gpu::vulkan::VKBackend::do_getQuery),

    (&::gpu::vulkan::VKBackend::do_resetStages),

    (&::gpu::vulkan::VKBackend::do_disableContextViewCorrection),
    (&::gpu::vulkan::VKBackend::do_restoreContextViewCorrection),
    (&::gpu::vulkan::VKBackend::do_disableContextStereo),
    (&::gpu::vulkan::VKBackend::do_restoreContextStereo),

    (&::gpu::vulkan::VKBackend::do_runLambda),

    (&::gpu::vulkan::VKBackend::do_startNamedCall),
    (&::gpu::vulkan::VKBackend::do_stopNamedCall),

    (&::gpu::vulkan::VKBackend::do_glUniform1f),
    (&::gpu::vulkan::VKBackend::do_glUniform2f),
    (&::gpu::vulkan::VKBackend::do_glUniform3f),
    (&::gpu::vulkan::VKBackend::do_glUniform4f),
    (&::gpu::vulkan::VKBackend::do_glColor4f),

    (&::gpu::vulkan::VKBackend::do_pushProfileRange),
    (&::gpu::vulkan::VKBackend::do_popProfileRange),
} };
#endif
