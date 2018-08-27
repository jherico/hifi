//
//  Created by Bradley Austin Davis on 2015/05/21
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "VKCubeWindow.h"

#include <set>
#include <GLMHelpers.h>
#include <glm/gtc/matrix_transform.hpp>

#include <QtCore/QCoreApplication>
#include <QtGui/qevent.h>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

#include <shared/Shapes.h>
#include <shared/RateCounter.h>
#include <SharedUtil.h>

#include <vk/Pipelines.h>

class CloseEventFilter : public QObject {
    Q_OBJECT
public:
    CloseEventFilter(QObject *parent) : QObject(parent) {}

protected:
    bool eventFilter(QObject *obj, QEvent *event) {
        if (event->type() == QEvent::Close) {
            QWindow* window = dynamic_cast<QWindow*>(obj);
            if (window) {
                qApp->quit();
                return true;
            }

        }
        return QObject::eventFilter(obj, event);
    }
};

static std::string sourcePath() {
    static std::string result;
    static std::once_flag once;
    std::call_once(once, [&] {
        result = QFileInfo(__FILE__).path().toStdString();
    });
    return result;
}

static uint32_t LOCATION_POSITION = 0;
static uint32_t LOCATION_COLOR = 1;
static uint32_t LOCATION_NORMAL = 2;
static uint32_t LOCATION_TRANSFORM = 3;

struct Vertex {
    vec4 pos;
    vec4 color;
    vec4 normal;
};

CubeWindow::CubeWindow(QScreen* screen) : Parent(screen) {
    installEventFilter(new CloseEventFilter(this));

    _extent = { 800, 600 };
    setGeometry(100 + 1920, 100, _extent.width, _extent.height);
    show();

    // Create the Vulkan _context
    _context.setValidationEnabled(true);
    _context.requireExtensions({ VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME });
    _context.requireDeviceExtensions({ VK_KHR_SWAPCHAIN_EXTENSION_NAME });
    _context.create();

    createSwapchain();

    updateProjection();
    viewport.height = (float)_extent.height;
    viewport.width = (float)_extent.width;
    scissor.extent = _extent;

    semaphores.renderComplete = _device.createSemaphore({});
    semaphores.presentComplete = _device.createSemaphore({});

    init_command_pool();
    init_uniform_buffer();
    init_descriptor_and_pipeline_layouts();
    init_vertex_buffer();
    init_transform_buffer();
    init_descriptor_pool();
    init_descriptor_set();
    init_pipeline();


    record_draw_commands();

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [&] { draw(); });
    timer->setInterval(0);
    timer->start();
}

CubeWindow::~CubeWindow() {
    _device.destroyPipeline(pipeline);
    vertex_buffer.destroy();
    for (const auto& desc_layout : descriptSetLayouts) {
        _device.destroyDescriptorSetLayout(desc_layout);
    }
    descriptSetLayouts.clear();
    _device.destroyPipelineLayout(pipeline_layout);
    uniform_data.destroy();
}

void CubeWindow::draw() {
    static RateCounter<> fps;
    static quint64 fpsReportTimestamp = usecTimestampNow();

    doEvery(fpsReportTimestamp, 3, [] {
        qDebug() << "FPS " << fps.rate();
    });

    fps.increment();

    try {
        if (width() != _extent.width || height() != _extent.height) {
            return;
        }
        update_uniform_buffer();
        update_transform_buffer();
        auto result = _swapchain.acquireNextImage(semaphores.presentComplete);
        if (result.result == vk::Result::eSuboptimalKHR) {
        }
        auto currentImage = result.value;
        vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eBottomOfPipe;
        vk::SubmitInfo submitInfo{ 1, &semaphores.presentComplete, &pipe_stage_flags, 1, drawCommandBuffers.data() + currentImage, 1, &semaphores.renderComplete };
        _context.queue.submit(submitInfo, {});
        _swapchain.queuePresent(semaphores.renderComplete);
    }  catch (const std::system_error& error) {
        if ((std::error_code)vk::Result::eErrorDeviceLost != error.code()) {
            throw;
        } else {
            qDebug() << "Device lost";
        }
    }
}

void CubeWindow::record_draw_commands() {
    using namespace vk;

    static const std::array<ClearValue, 2> clearValues{
        ClearColorValue(std::array<float, 4Ui64>{ { 0.2f, 0.2f, 0.2f, 0.2f } }),
        ClearDepthStencilValue({ 1.0f, 0 }),
    };

    // FIXME properly destroy the old command buffers
    drawCommandBuffers = _device.allocateCommandBuffers({ commandPool, vk::CommandBufferLevel::ePrimary, _swapchain.imageCount });
    for (size_t i = 0; i < _swapchain.imageCount; ++i) {
        const auto& drawCommandBuffer = drawCommandBuffers[i];
        const auto& framebuffer = _framebuffers[i];
        drawCommandBuffer.reset(CommandBufferResetFlagBits::eReleaseResources);
        drawCommandBuffer.begin({ vk::CommandBufferUsageFlagBits::eSimultaneousUse });
        drawCommandBuffer.beginRenderPass({ _renderPass, framebuffer, Rect2D{ {}, _extent }, (uint32_t)clearValues.size(), clearValues.data() }, SubpassContents::eInline);
        drawCommandBuffer.setViewport(0, viewport);
        drawCommandBuffer.setScissor(0, scissor);
        drawCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        drawCommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, descriptorSet, nullptr);
        drawCommandBuffer.bindVertexBuffers(0, { vertex_buffer.buffer, transform_buffer.buffer }, { 0, 0 });
        drawCommandBuffer.draw(faceCount * 3, GRID_SIZE, 0, 0);
        drawCommandBuffer.endRenderPass();
        drawCommandBuffer.end();
    }
}

void CubeWindow::init_command_pool() {
    vk::CommandPoolCreateInfo cmdPoolInfo;
    cmdPoolInfo.queueFamilyIndex = _context.queueIndices.graphics;
    cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPool = _device.createCommandPool(cmdPoolInfo);
}

void CubeWindow::init_uniform_buffer() {
    uniform_data = _context.createUniformBuffer(sizeof(Transform));
    update_uniform_buffer();
}

void CubeWindow::update_uniform_buffer() {
    float t = secTimestampNow() / 3.0f;
    _transform.view = glm::lookAt(
        glm::vec3(cos(t) * 5, 0.3, sin(t) * 5), // Camera is at (5,3,10), in World Space
        glm::vec3(0, 0, 0),  // and looks at the origin
        glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );
    
    _transform.mvp = _transform.projection * _transform.view;
    memcpy(uniform_data.mapped, &_transform, sizeof(Transform));
}

void CubeWindow::update_transform_buffer() {
    for (int x = 0; x < GRID_X; ++x) {
        for (int y = 0; y < GRID_Y; ++y) {
            for (int z = 0; z < GRID_Z; ++z) {
                size_t offset = x * GRID_Y * GRID_Z + y * GRID_Z + z;
                const auto& r = rotations[offset];
                matrices[offset] = glm::rotate(mat4(), 0.02f / (r.w + 1), vec3(r)) * matrices[offset];
            }
        }
    }
    static auto TRANSFORM_BUFFER_SIZE = GRID_SIZE * sizeof(mat4);
    memcpy(transform_buffer.mapped, matrices.data(), TRANSFORM_BUFFER_SIZE);
}

void CubeWindow::updateProjection() {
    auto size = this->size();
    float aspect = (float)size.width() / (float)size.height();
    _transform.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
}

void CubeWindow::resizeFramebuffer() {
    Parent::resizeFramebuffer();
    updateProjection();
    viewport.height = (float)_extent.height;
    viewport.width = (float)_extent.width;
    scissor.extent = _extent;
    record_draw_commands();
}

void CubeWindow::init_descriptor_and_pipeline_layouts() {
    vk::DescriptorSetLayoutBinding layout_binding;
    layout_binding
        .setBinding(0)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(1)
        .setStageFlags(vk::ShaderStageFlagBits::eVertex);

    vk::DescriptorSetLayoutCreateInfo descriptor_layout;
    descriptor_layout
        .setBindingCount(1)
        .setPBindings(&layout_binding);

    descriptSetLayouts.resize(NUM_DESCRIPTOR_SETS);
    descriptSetLayouts[0] = _device.createDescriptorSetLayout(descriptor_layout);
}

static const auto BUFFER_MEMORY_FLAGS = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

void CubeWindow::init_transform_buffer() {
    transform_buffer = _context.createBuffer(
        vk::BufferUsageFlagBits::eVertexBuffer,
        sizeof(mat4) * GRID_SIZE,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    transform_buffer.map();


    {
        matrices.resize(GRID_SIZE);
        rotations.resize(GRID_SIZE);
        for (int x = 0; x < GRID_X; ++x) {
            for (int y = 0; y < GRID_Y; ++y) {
                for (int z = 0; z < GRID_Z; ++z) {
                    size_t offset = x * GRID_Y * GRID_Z + y * GRID_Z + z;
                    vec3 position = vec3(x - GRID_X / 2, y - GRID_Y / 2, z - GRID_Z / 2);
                    matrices[offset] = glm::scale(glm::translate(mat4(), position), vec3(0.2f));
                    rotations[offset] = vec4(
                        glm::normalize(vec3(randFloat() - 0.5f, randFloat() - 0.5f, randFloat() - 0.5f)),
                        glm::length(position)
                    );
                }
            }
        }
        memcpy(transform_buffer.mapped, matrices.data(), sizeof(mat4) * matrices.size());
    }
}



void CubeWindow::init_vertex_buffer() {
    auto shape = geometry::icosahedron();

    using namespace geometry;
    std::vector<Vertex> vertices;

    faceCount = shape.faces.size();
    size_t faceIndexCount = geometry::triangulatedFaceIndexCount<3>();
    vertices.reserve(3 * faceCount * 2);

    srand(usecTimestampNow());
    vec4 color(randFloat(), randFloat(), randFloat(), 1.0f);
    for (size_t f = 0; f < faceCount; ++f) {
        const Face<3>& face = shape.faces[f];
        vec3 normal = shape.getFaceNormal(f);
        // Create the vertices for the face
        vertices.push_back({ vec4(shape.vertices[face[0]], 1.0), color, vec4(normal, 1.0f) });
        vertices.push_back({ vec4(shape.vertices[face[2]], 1.0), color, vec4(normal, 1.0f) });
        vertices.push_back({ vec4(shape.vertices[face[1]], 1.0), color, vec4(normal, 1.0f) });
    }

    vertex_buffer = _context.stageToDeviceBuffer(vk::BufferUsageFlagBits::eVertexBuffer, vertices);
}

void CubeWindow::init_descriptor_pool() {
    vk::DescriptorPoolSize type_count{ vk::DescriptorType::eUniformBuffer, 1 };
    descriptorPool = _device.createDescriptorPool({ {}, 1, 1, &type_count });
}

void CubeWindow::init_descriptor_set() {
    descriptorSet = _device.allocateDescriptorSets({ descriptorPool, (uint32_t)descriptSetLayouts.size(), descriptSetLayouts.data() });
    _device.updateDescriptorSets({ { descriptorSet[0], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &uniform_data.descriptor } }, nullptr);
}

void CubeWindow::init_pipeline(VkBool32 include_vi) {
    /* Now use the descriptor layout to create a pipeline layout */
    pipeline_layout = _device.createPipelineLayout({ {},  (uint32_t)descriptSetLayouts.size(), descriptSetLayouts.data() });
    vks::pipelines::GraphicsPipelineBuilder builder{ _device, pipeline_layout, _renderPass };
    builder.vertexInputState.bindingDescriptions.push_back({ 0, sizeof(Vertex), vk::VertexInputRate::eVertex });
    builder.vertexInputState.attributeDescriptions.push_back({ LOCATION_POSITION, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, pos) });
    builder.vertexInputState.attributeDescriptions.push_back({ LOCATION_COLOR, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color) });
    builder.vertexInputState.attributeDescriptions.push_back({ LOCATION_NORMAL, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, normal) });

    builder.vertexInputState.bindingDescriptions.push_back({ 1, sizeof(mat4), vk::VertexInputRate::eInstance });
    for (int i = 0; i < 4; ++i) {
        builder.vertexInputState.attributeDescriptions.push_back({ LOCATION_TRANSFORM + i, 1, vk::Format::eR32G32B32A32Sfloat, i * sizeof(vec4) });
    }

    builder.loadShader(sourcePath() + "/cube.vert.spv", vk::ShaderStageFlagBits::eVertex);
    builder.loadShader(sourcePath() + "/cube.frag.spv", vk::ShaderStageFlagBits::eFragment);
    pipeline = builder.create(_context.pipelineCache);
}

#include "VKCubeWindow.moc"

