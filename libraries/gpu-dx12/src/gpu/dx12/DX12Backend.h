//
//  DX12Backend.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLBackend_h
#define hifi_gpu_gl_GLBackend_h

#include <assert.h>
#include <functional>
#include <memory>
#include <bitset>
#include <queue>
#include <utility>
#include <list>
#include <array>

#include <QtCore/QLoggingCategory>

#include <gpu/Forward.h>
#include <gpu/Context.h>
#include <GLMHelpers.h>
#include <Transform.h>

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <client.h>


Q_DECLARE_LOGGING_CATEGORY(trace_render_gpu_dx)
Q_DECLARE_LOGGING_CATEGORY(trace_render_gpu_dx_detail)


// Different versions for the stereo drawcall
// Current preferred is  "instanced" which draw the shape twice but instanced and rely on clipping plane to draw left/right side only
//#define GPU_STEREO_TECHNIQUE_DOUBLED_SIMPLE
//#define GPU_STEREO_TECHNIQUE_DOUBLED_SMARTER
#define GPU_STEREO_TECHNIQUE_INSTANCED


// Let these be configured by the one define picked above
#ifdef GPU_STEREO_TECHNIQUE_DOUBLED_SIMPLE
#define GPU_STEREO_DRAWCALL_DOUBLED
#endif

#ifdef GPU_STEREO_TECHNIQUE_DOUBLED_SMARTER
#define GPU_STEREO_DRAWCALL_DOUBLED
#define GPU_STEREO_CAMERA_BUFFER
#endif

#ifdef GPU_STEREO_TECHNIQUE_INSTANCED
#define GPU_STEREO_DRAWCALL_INSTANCED
#define GPU_STEREO_CAMERA_BUFFER
#endif

namespace gpu { namespace dx12 {

class DX12Framebuffer;
class DX12Buffer;
class DX12Texture;
class DX12Query;

class DX12Backend : public Backend, public std::enable_shared_from_this<DX12Backend> {
    // Context Backend static interface required
    friend class gpu::Context;
    static void init();
    static BackendPointer createBackend();

public:
    explicit DX12Backend(bool syncCache);
    DX12Backend();

public:
    static bool makeProgram(Shader& shader, const Shader::BindingSet& slotBindings = Shader::BindingSet());

    virtual ~DX12Backend();

    void setCameraCorrection(const Mat4& correction);
    void render(const Batch& batch) final override;

    // This call synchronize the Full Backend cache with the current DX12State
    // THis is only intended to be used when mixing raw gl calls with the gpu api usage in order to sync
    // the gpu::Backend state with the true gl state which has probably been messed up by these ugly naked gl calls
    // Let's try to avoid to do that as much as possible!
    void syncCache() final override;

    // This is the ugly "download the pixels to sysmem for taking a snapshot"
    // Just avoid using it, it's ugly and will break performances
    virtual void downloadFramebuffer(const FramebufferPointer& srcFramebuffer,
                                     const Vec4i& region, QImage& destImage) final override;


    // this is the maximum numeber of available input buffers
    size_t getNumInputBuffers() const { return _input._invalidBuffers.size(); }

    // this is the maximum per shader stage on the low end apple
    // TODO make it platform dependant at init time
    static const int MAX_NUM_UNIFORM_BUFFERS = 12;
    size_t getMaxNumUniformBuffers() const { return MAX_NUM_UNIFORM_BUFFERS; }

    // this is the maximum per shader stage on the low end apple
    // TODO make it platform dependant at init time
    static const int MAX_NUM_RESOURCE_BUFFERS = 16;
    size_t getMaxNumResourceBuffers() const { return MAX_NUM_RESOURCE_BUFFERS; }
    static const int MAX_NUM_RESOURCE_TEXTURES = 16;
    size_t getMaxNumResourceTextures() const { return MAX_NUM_RESOURCE_TEXTURES; }

    // Draw Stage
    void do_draw(const Batch& batch, size_t paramOffset);
    void do_drawIndexed(const Batch& batch, size_t paramOffset);
    void do_drawInstanced(const Batch& batch, size_t paramOffset);
    void do_drawIndexedInstanced(const Batch& batch, size_t paramOffset);
    void do_multiDrawIndirect(const Batch& batch, size_t paramOffset);
    void do_multiDrawIndexedIndirect(const Batch& batch, size_t paramOffset);

    // Input Stage
    void do_setInputFormat(const Batch& batch, size_t paramOffset);
    void do_setInputBuffer(const Batch& batch, size_t paramOffset);
    void do_setIndexBuffer(const Batch& batch, size_t paramOffset);
    void do_setIndirectBuffer(const Batch& batch, size_t paramOffset);
    void do_generateTextureMips(const Batch& batch, size_t paramOffset);

    // Transform Stage
    void do_setModelTransform(const Batch& batch, size_t paramOffset);
    void do_setViewTransform(const Batch& batch, size_t paramOffset);
    void do_setProjectionTransform(const Batch& batch, size_t paramOffset);
    void do_setViewportTransform(const Batch& batch, size_t paramOffset);
    void do_setDepthRangeTransform(const Batch& batch, size_t paramOffset);

    // Uniform Stage
    void do_setUniformBuffer(const Batch& batch, size_t paramOffset);

    // Resource Stage
    void do_setResourceBuffer(const Batch& batch, size_t paramOffset);
    void do_setResourceTexture(const Batch& batch, size_t paramOffset);

    // Pipeline Stage
    void do_setPipeline(const Batch& batch, size_t paramOffset);

    // Output stage
    void do_setFramebuffer(const Batch& batch, size_t paramOffset);
    void do_clearFramebuffer(const Batch& batch, size_t paramOffset);
    void do_blit(const Batch& batch, size_t paramOffset);

    // Query section
    void do_beginQuery(const Batch& batch, size_t paramOffset);
    void do_endQuery(const Batch& batch, size_t paramOffset);
    void do_getQuery(const Batch& batch, size_t paramOffset);

    // Reset stages
    void do_resetStages(const Batch& batch, size_t paramOffset);

    
    void do_disableContextViewCorrection(const Batch& batch, size_t paramOffset);
    void do_restoreContextViewCorrection(const Batch& batch, size_t paramOffset);

    void do_disableContextStereo(const Batch& batch, size_t paramOffset);
    void do_restoreContextStereo(const Batch& batch, size_t paramOffset);

    void do_runLambda(const Batch& batch, size_t paramOffset);

    void do_startNamedCall(const Batch& batch, size_t paramOffset);
    void do_stopNamedCall(const Batch& batch, size_t paramOffset);

    static const int MAX_NUM_ATTRIBUTES = Stream::NUM_INPUT_SLOTS;
    // The drawcall Info attribute  channel is reserved and is the upper bound for the number of availables Input buffers
    static const int MAX_NUM_INPUT_BUFFERS = Stream::DRAW_CALL_INFO;

    void do_pushProfileRange(const Batch& batch, size_t paramOffset);
    void do_popProfileRange(const Batch& batch, size_t paramOffset);

    // TODO: As long as we have gl calls explicitely issued from interface
    // code, we need to be able to record and batch these calls. THe long 
    // term strategy is to get rid of any DX12 calls in favor of the HIFI GPU API
    void do_glUniform1i(const Batch& batch, size_t paramOffset);
    void do_glUniform1f(const Batch& batch, size_t paramOffset);
    void do_glUniform2f(const Batch& batch, size_t paramOffset);
    void do_glUniform3f(const Batch& batch, size_t paramOffset);
    void do_glUniform4f(const Batch& batch, size_t paramOffset);
    void do_glUniform3fv(const Batch& batch, size_t paramOffset);
    void do_glUniform4fv(const Batch& batch, size_t paramOffset);
    void do_glUniform4iv(const Batch& batch, size_t paramOffset);
    void do_glUniformMatrix3fv(const Batch& batch, size_t paramOffset);
    void do_glUniformMatrix4fv(const Batch& batch, size_t paramOffset);

    void do_glColor4f(const Batch& batch, size_t paramOffset);

    // The State setters called by the DX12State::Commands when a new state is assigned
    void do_setStateFillMode(int32 mode);
    void do_setStateCullMode(int32 mode);
    void do_setStateFrontFaceClockwise(bool isClockwise);
    void do_setStateDepthClampEnable(bool enable);
    void do_setStateScissorEnable(bool enable);
    void do_setStateMultisampleEnable(bool enable);
    void do_setStateAntialiasedLineEnable(bool enable);
    void do_setStateDepthBias(Vec2 bias);
    void do_setStateDepthTest(State::DepthTest test);
    void do_setStateStencil(State::StencilActivation activation, State::StencilTest frontTest, State::StencilTest backTest);
    void do_setStateAlphaToCoverageEnable(bool enable);
    void do_setStateSampleMask(uint32 mask);
    void do_setStateBlend(State::BlendFunction blendFunction);
    void do_setStateColorWriteMask(uint32 mask);
    void do_setStateBlendFactor(const Batch& batch, size_t paramOffset);
    void do_setStateScissorRect(const Batch& batch, size_t paramOffset);

    void* getFramebufferID(const FramebufferPointer& framebuffer);
    ComPtr<ID3D12Resource> getTextureID(const TexturePointer& texture);
    void* getBufferID(const Buffer& buffer);
    void* getQueryID(const QueryPointer& query);

    DX12Framebuffer* syncGPUObject(const Framebuffer& framebuffer);
    DX12Buffer* syncGPUObject(const Buffer& buffer);
    DX12Texture* syncGPUObject(const TexturePointer& texture);
    DX12Query* syncGPUObject(const Query& query);
    //virtual bool isTextureReady(const TexturePointer& texture);

    void releaseBuffer(void* id, Size size) const;
    void releaseExternalTexture(void* id, const Texture::ExternalRecycler& recycler) const;
    void releaseTexture(void* id, Size size) const;
    void releaseFramebuffer(void* id) const;
    void releaseShader(void* id) const;
    void releaseProgram(void* id) const;
    void releaseQuery(void* id) const;
    void queueLambda(const std::function<void()> lambda) const;

    bool isTextureManagementSparseEnabled() const override { return false; }

protected:
    void recycle() const override;

    static const size_t INVALID_OFFSET = (size_t)-1;
    bool _inRenderTransferPass { false };
    int32_t _uboAlignment { 0 };
    int _currentDraw { -1 };

    std::list<std::string> profileRanges;
    mutable Mutex _trashMutex;
    mutable std::list<std::pair<void*, Size>> _buffersTrash;
    mutable std::list<std::pair<void*, Size>> _texturesTrash;
    mutable std::list<std::pair<void*, Texture::ExternalRecycler>> _externalTexturesTrash;
    mutable std::list<void*> _framebuffersTrash;
    mutable std::list<void*> _shadersTrash;
    mutable std::list<void*> _programsTrash;
    mutable std::list<void*> _queriesTrash;
    mutable std::list<std::function<void()>> _lambdaQueue;

    void renderPassTransfer(const Batch& batch);
    void renderPassDraw(const Batch& batch);

#ifdef GPU_STEREO_DRAWCALL_DOUBLED
    void setupStereoSide(int side);
#endif

    virtual void initInput();
    virtual void killInput();
    virtual void syncInputStateCache();
    virtual void resetInputStage();
    virtual void updateInput();

    struct InputStageState {
        bool _invalidFormat { true };
        Stream::FormatPointer _format;
        std::string _formatKey;

        typedef std::bitset<MAX_NUM_ATTRIBUTES> ActivationCache;
        ActivationCache _attributeActivation { 0 };

        typedef std::bitset<MAX_NUM_INPUT_BUFFERS> BuffersState;

        BuffersState _invalidBuffers{ 0 };
        BuffersState _attribBindingBuffers{ 0 };

        Buffers _buffers;
        Offsets _bufferOffsets;
        Offsets _bufferStrides;
        std::vector<void*> _bufferVBOs;

        glm::vec4 _colorAttribute{ 0.0f };

        BufferPointer _indexBuffer;
        Offset _indexBufferOffset { 0 };
        Type _indexBufferType { UINT32 };
        
        BufferPointer _indirectBuffer;
        Offset _indirectBufferOffset{ 0 };
        Offset _indirectBufferStride{ 0 };

        void* _defaultVAO { 0 };

        InputStageState() :
            _invalidFormat(true),
            _format(0),
            _formatKey(),
            _attributeActivation(0),
            _buffers(_invalidBuffers.size(), BufferPointer(0)),
            _bufferOffsets(_invalidBuffers.size(), 0),
            _bufferStrides(_invalidBuffers.size(), 0),
            _bufferVBOs(_invalidBuffers.size(), 0) {}
    } _input;

    void initTransform();
    void killTransform();
    // Synchronize the state cache of this Backend with the actual real state of the DX12 Context
    void syncTransformStateCache();
    void updateTransform(const Batch& batch);
    void resetTransformStage();

    // Allows for correction of the camera pose to account for changes
    // between the time when a was recorded and the time(s) when it is 
    // executed
    struct CameraCorrection {
        Mat4 correction;
        Mat4 correctionInverse;
    };

    struct TransformStageState {
#ifdef GPU_STEREO_CAMERA_BUFFER
        struct Cameras {
            TransformCamera _cams[2];

            Cameras() {};
            Cameras(const TransformCamera& cam) { memcpy(_cams, &cam, sizeof(TransformCamera)); };
            Cameras(const TransformCamera& camL, const TransformCamera& camR) { memcpy(_cams, &camL, sizeof(TransformCamera)); memcpy(_cams + 1, &camR, sizeof(TransformCamera)); };
        };

        using CameraBufferElement = Cameras;
#else
        using CameraBufferElement = TransformCamera;
#endif
        using TransformCameras = std::vector<CameraBufferElement>;

        TransformCamera _camera;
        TransformCameras _cameras;

        mutable std::map<std::string, void*> _drawCallInfoOffsets;

        void* _objectBuffer { 0 };
        void* _cameraBuffer { 0 };
        void* _drawCallInfoBuffer { 0 };
        void* _objectBufferTexture { 0 };
        size_t _cameraUboSize { 0 };
        bool _viewIsCamera{ false };
        bool _skybox { false };
        Transform _view;
        CameraCorrection _correction;
        bool _viewCorrectionEnabled{ true };


        Mat4 _projection;
        Vec4i _viewport { 0, 0, 1, 1 };
        Vec2 _depthRange { 0.0f, 1.0f };
        bool _invalidView { false };
        bool _invalidProj { false };
        bool _invalidViewport { false };

        bool _enabledDrawcallInfoBuffer{ false };

        using Pair = std::pair<size_t, size_t>;
        using List = std::list<Pair>;
        List _cameraOffsets;
        mutable List::const_iterator _camerasItr;
        mutable size_t _currentCameraOffset{ INVALID_OFFSET };

        void preUpdate(size_t commandIndex, const StereoState& stereo);
        void update(size_t commandIndex, const StereoState& stereo) const;
        void bindCurrentCamera(int stereoSide) const;
    } _transform;

    virtual void transferTransformState(const Batch& batch) const;

    struct UniformStageState {
        std::array<BufferPointer, MAX_NUM_UNIFORM_BUFFERS> _buffers;
        //Buffers _buffers {  };
    } _uniform;

    void releaseUniformBuffer(uint32_t slot);
    void resetUniformStage();

    // update resource cache and do the gl bind/unbind call with the current gpu::Buffer cached at slot s
    // This is using different gl object  depending on the gl version
    bool bindResourceBuffer(uint32_t slot, BufferPointer& buffer);
    void releaseResourceBuffer(uint32_t slot);

    // update resource cache and do the gl unbind call with the current gpu::Texture cached at slot s
    void releaseResourceTexture(uint32_t slot);

    void resetResourceStage();

    struct ResourceStageState {
        std::array<BufferPointer, MAX_NUM_RESOURCE_BUFFERS> _buffers;
        std::array<TexturePointer, MAX_NUM_RESOURCE_TEXTURES> _textures;
        //Textures _textures { { MAX_NUM_RESOURCE_TEXTURES } };
        int findEmptyTextureSlot() const;
    } _resource;

    size_t _commandIndex{ 0 };

    // Standard update pipeline check that the current Program and current State or good to go for a
    void updatePipeline();
    // Force to reset all the state fields indicated by the 'toBeReset" signature
    void resetPipelineState(State::Signature toBeReset);
    // Synchronize the state cache of this Backend with the actual real state of the DX12 Context
    void syncPipelineStateCache();
    void resetPipelineStage();

    struct PipelineStageState {
        PipelinePointer _pipeline;

        void* _program{ 0 };
        void* _cameraCorrectionLocation{ (void*)-1 };
        void* _programShader{ nullptr };
        bool _invalidProgram{ false };

        BufferView _cameraCorrectionBuffer{ gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(CameraCorrection), nullptr)) };
        BufferView _cameraCorrectionBufferIdentity{ gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(CameraCorrection), nullptr)) };

        State::Data _stateCache{ State::DEFAULT };
        State::Signature _stateSignatureCache{ 0 };

        void* _state{ nullptr };
        bool _invalidState{ false };

        PipelineStageState() {
            _cameraCorrectionBuffer.edit<CameraCorrection>() = CameraCorrection();
            _cameraCorrectionBufferIdentity.edit<CameraCorrection>() = CameraCorrection();
            //_cameraCorrectionBufferIdentity._buffer->flush();
        }
    } _pipeline;

#if 0

    // Backend dependant compilation of the shader
    virtual DX12Shader* compileBackendProgram(const Shader& program);
    virtual DX12Shader* compileBackendShader(const Shader& shader);
    virtual std::string getBackendShaderHeader() const;
    virtual void makeProgramBindings(ShaderObject& shaderObject);
    class ElementResource {
    public:
        gpu::Element _element;
        uint16 _resource;
        ElementResource(Element&& elem, uint16 resource) : _element(elem), _resource(resource) {}
    };
    ElementResource getFormatFromGLUniform(DX12enum gltype);
    static const DX12int UNUSED_SLOT {-1};
    static bool isUnusedSlot(DX12int binding) { return (binding == UNUSED_SLOT); }
    virtual int makeUniformSlots(void* glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& uniforms, Shader::SlotSet& textures, Shader::SlotSet& samplers);
    virtual int makeUniformBlockSlots(void* glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& buffers);
    virtual int makeResourceBufferSlots(void* glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& resourceBuffers);
    virtual int makeInputSlots(void* glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& inputs);
    virtual int makeOutputSlots(void* glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& outputs);


    // Synchronize the state cache of this Backend with the actual real state of the DX12 Context
    void syncOutputStateCache();
    void resetOutputStage();
    
    struct OutputStageState {
        FramebufferPointer _framebuffer { nullptr };
        void* _drawFBO { 0 };
    } _output;

    void resetQueryStage();
    struct QueryStageState {
        uint32_t _rangeQueryDepth { 0 };
    } _queryStage;

    void resetStages();

    struct TextureManagementStageState {
        bool _sparseCapable { false };
    } _textureManagement;
    virtual void initTextureManagementStage() {}

    friend class DX12State;
    friend class DX12Texture;
    friend class DX12Shader;
#endif
    typedef void (DX12Backend::*CommandCall)(const Batch&, size_t);
    static CommandCall _commandCalls[Batch::NUM_COMMANDS];

    static const std::string DX12_VERSION;
    const std::string& getVersion() const override { return DX12_VERSION; }
};

} }

#endif


#if 0
//
//  DX1245Backend.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_gpu_45_GL45Backend_h
#define hifi_gpu_45_GL45Backend_h

#include "../gl/DX12Backend.h"
#include "../gl/DX12Texture.h"
#include <thread>

#define INCREMENTAL_TRANSFER 0
#define GPU_SSBO_TRANSFORM_OBJECT 1

namespace gpu { namespace gl45 {

using namespace gpu::gl;
using TextureWeakPointer = std::weak_ptr<Texture>;

class DX1245Backend : public DX12Backend {
    using Parent = DX12Backend;
    // Context Backend static interface required
    friend class Context;

public:

#ifdef GPU_SSBO_TRANSFORM_OBJECT
    static const DX12int TRANSFORM_OBJECT_SLOT  { 14 }; // SSBO binding slot
#else
    static const DX12int TRANSFORM_OBJECT_SLOT  { 31 }; // TBO binding slot
#endif

    explicit DX1245Backend(bool syncCache) : Parent(syncCache) {}
    DX1245Backend() : Parent() {}
    virtual ~DX1245Backend() {
        // call resetStages here rather than in ~DX12Backend dtor because it will call releaseResourceBuffer
        // which is pure virtual from DX12Backend's dtor.
        resetStages();
    }


    class DX1245Texture : public DX12Texture {
        using Parent = DX12Texture;
        friend class DX1245Backend;
        static void* allocate(const Texture& texture);
    protected:
        DX1245Texture(const std::weak_ptr<DX12Backend>& backend, const Texture& texture);
        void generateMips() const override;
        Size copyMipFaceLinesFromTexture(uint16_t mip, uint8_t face, const uvec3& size, uint32_t yOffset, DX12enum internalFormat, DX12enum format, DX12enum type, Size sourceSize, const void* sourcePointer) const override;
        void syncSampler() const override;
    };

    //
    // Textures that have fixed allocation sizes and cannot be managed at runtime
    //

    class DX1245FixedAllocationTexture : public DX1245Texture {
        using Parent = DX1245Texture;
        friend class DX1245Backend;

    public:
        DX1245FixedAllocationTexture(const std::weak_ptr<DX12Backend>& backend, const Texture& texture);
        ~DX1245FixedAllocationTexture();

    protected:
        Size size() const override { return _size; }
        void allocateStorage() const;
        void syncSampler() const override;
        const Size _size { 0 };
    };

    class DX1245AttachmentTexture : public DX1245FixedAllocationTexture {
        using Parent = DX1245FixedAllocationTexture;
        friend class DX1245Backend;
    protected:
        DX1245AttachmentTexture(const std::weak_ptr<DX12Backend>& backend, const Texture& texture);
        ~DX1245AttachmentTexture();
    };

    class DX1245StrictResourceTexture : public DX1245FixedAllocationTexture {
        using Parent = DX1245FixedAllocationTexture;
        friend class DX1245Backend;
    protected:
        DX1245StrictResourceTexture(const std::weak_ptr<DX12Backend>& backend, const Texture& texture);
        ~DX1245StrictResourceTexture();
    };

    //
    // Textures that can be managed at runtime to increase or decrease their memory load
    //

    class DX1245VariableAllocationTexture : public DX1245Texture, public DX12VariableAllocationSupport {
        using Parent = DX1245Texture;
        friend class DX1245Backend;
        using PromoteLambda = std::function<void()>;


    protected:
        DX1245VariableAllocationTexture(const std::weak_ptr<DX12Backend>& backend, const Texture& texture);
        ~DX1245VariableAllocationTexture();

        Size size() const override { return _size; }

        Size copyMipFaceLinesFromTexture(uint16_t mip, uint8_t face, const uvec3& size, uint32_t yOffset, DX12enum internalFormat, DX12enum format, DX12enum type, Size sourceSize, const void* sourcePointer) const override;
        void copyTextureMipsInGPUMem(void* srcId, void* destId, uint16_t srcMipOffset, uint16_t destMipOffset, uint16_t populatedMips) override;

    };

    class DX1245ResourceTexture : public DX1245VariableAllocationTexture {
        using Parent = DX1245VariableAllocationTexture;
        friend class DX1245Backend;
    protected:
        DX1245ResourceTexture(const std::weak_ptr<DX12Backend>& backend, const Texture& texture);

        void syncSampler() const override;
        void promote() override;
        void demote() override;
        void populateTransferQueue() override;


        void allocateStorage(uint16 mip);
        Size copyMipsFromTexture();
    };

#if 0
    class DX1245SparseResourceTexture : public DX1245VariableAllocationTexture {
        using Parent = DX1245VariableAllocationTexture;
        friend class DX1245Backend;
        using TextureTypeFormat = std::pair<DX12enum, DX12enum>;
        using PageDimensions = std::vector<uvec3>;
        using PageDimensionsMap = std::map<TextureTypeFormat, PageDimensions>;
        static PageDimensionsMap pageDimensionsByFormat;
        static Mutex pageDimensionsMutex;

        static bool isSparseEligible(const Texture& texture);
        static PageDimensions getPageDimensionsForFormat(const TextureTypeFormat& typeFormat);
        static PageDimensions getPageDimensionsForFormat(DX12enum type, DX12enum format);
        static const uint32_t DEFAULT_PAGE_DIMENSION = 128;
        static const uint32_t DEFAULT_MAX_SPARSE_LEVEL = 0xFFFF;

    protected:
        DX1245SparseResourceTexture(const std::weak_ptr<DX12Backend>& backend, const Texture& texture);
        ~DX1245SparseResourceTexture();
        uint32 size() const override { return _allocatedPages * _pageBytes; }
        void promote() override;
        void demote() override;

    private:
        uvec3 getPageCounts(const uvec3& dimensions) const;
        uint32_t getPageCount(const uvec3& dimensions) const;

        uint32_t _allocatedPages { 0 };
        uint32_t _pageBytes { 0 };
        uvec3 _pageDimensions { DEFAULT_PAGE_DIMENSION };
        void* _maxSparseLevel { DEFAULT_MAX_SPARSE_LEVEL };
    };
#endif


protected:

    void recycle() const override;

    void* getFramebufferID(const FramebufferPointer& framebuffer) override;
    DX12Framebuffer* syncGPUObject(const Framebuffer& framebuffer) override;

    void* getBufferID(const Buffer& buffer) override;
    DX12Buffer* syncGPUObject(const Buffer& buffer) override;

    DX12Texture* syncGPUObject(const TexturePointer& texture) override;

    void* getQueryID(const QueryPointer& query) override;
    DX12Query* syncGPUObject(const Query& query) override;

    // Draw Stage
    void do_draw(const Batch& batch, size_t paramOffset) override;
    void do_drawIndexed(const Batch& batch, size_t paramOffset) override;
    void do_drawInstanced(const Batch& batch, size_t paramOffset) override;
    void do_drawIndexedInstanced(const Batch& batch, size_t paramOffset) override;
    void do_multiDrawIndirect(const Batch& batch, size_t paramOffset) override;
    void do_multiDrawIndexedIndirect(const Batch& batch, size_t paramOffset) override;

    // Input Stage
    void resetInputStage() override;
    void updateInput() override;

    // Synchronize the state cache of this Backend with the actual real state of the DX12 Context
    void transferTransformState(const Batch& batch) const override;
    void initTransform() override;
    void updateTransform(const Batch& batch) override;

    // Resource Stage
    bool bindResourceBuffer(uint32_t slot, BufferPointer& buffer) override;
    void releaseResourceBuffer(uint32_t slot) override;

    // Output stage
    void do_blit(const Batch& batch, size_t paramOffset) override;

    // Shader Stage
    std::string getBackendShaderHeader() const override;
    void makeProgramBindings(ShaderObject& shaderObject) override;
    int makeResourceBufferSlots(void* glprogram, const Shader::BindingSet& slotBindings,Shader::SlotSet& resourceBuffers) override;

    // Texture Management Stage
    void initTextureManagementStage() override;
};

} }

Q_DECLARE_LOGGING_CATEGORY(gpugl45logging)

#endif

#endif