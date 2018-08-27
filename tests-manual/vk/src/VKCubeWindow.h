//
//  Created by Bradley Austin Davis on 2015/05/21
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <mutex>
#include <QtGui/QWindow>

#include <GLMHelpers.h>

#include <vk/Context.h>
#include <vk/VKWindow.h>

#define FENCE_TIMEOUT 100000000
#define NUM_DESCRIPTOR_SETS 1

#define GRID_X 5
#define GRID_Y 5
#define GRID_Z 5
#define GRID_SIZE (GRID_X * GRID_Y * GRID_Z)

class CubeWindow : public VKWindow {
    using Parent = VKWindow;
    Q_OBJECT
public:
    CubeWindow(QScreen* screen = nullptr);
    virtual ~CubeWindow();
    void draw();

private:
    friend class VkCloseEventFilter;

    struct Semaphores {
        vk::Semaphore renderComplete;
        vk::Semaphore presentComplete;
    } semaphores;

    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> drawCommandBuffers;
    vk::Pipeline pipeline;
    vk::PipelineLayout pipeline_layout;
    vks::Buffer vertex_buffer;
    vks::Buffer transform_buffer;
    vks::Buffer uniform_data;
    
    struct Transform {
        glm::mat4 mvp;
        glm::mat4 projection;
        glm::mat4 view;
    } _transform;
    glm::mat4 Model;
    std::array<mat4, GRID_SIZE> transforms;
    size_t faceCount { 0 };
    std::vector<mat4> matrices;
    std::vector<vec4> rotations;
    std::vector<vk::DescriptorSetLayout> descriptSetLayouts;
    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> descriptorSet;

    vk::Viewport viewport;
    vk::Rect2D scissor;

    virtual void resizeFramebuffer() override;
    void updateProjection();

    void init_command_pool();
    void init_uniform_buffer();
    void init_descriptor_and_pipeline_layouts();
    void init_transform_buffer();
    void init_vertex_buffer();
    void init_descriptor_pool();
    void init_descriptor_set();
    void init_pipeline(VkBool32 include_vi = true);

    void update_uniform_buffer();
    void update_transform_buffer();
    void record_draw_commands();
};


