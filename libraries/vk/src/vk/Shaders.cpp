#include "Shaders.h"

#include <shared/Storage.h>

vk::ShaderModule vks::shaders::loadShaderModule(const vk::Device& device, const std::string& filename) {
    vk::ShaderModule result;
    {
        auto storage = std::make_shared<storage::FileStorage>(filename.c_str());
        result = device.createShaderModule({ {}, storage->size(), (const uint32_t*)storage->data() });
    }
    return result;
}

// Load a SPIR-V shader
vk::PipelineShaderStageCreateInfo vks::shaders::loadShader(const vk::Device& device, const std::string& fileName, vk::ShaderStageFlagBits stage, const char* entryPoint) {
    vk::PipelineShaderStageCreateInfo shaderStage;
    shaderStage.stage = stage;
    shaderStage.module = loadShaderModule(device, fileName);
    shaderStage.pName = entryPoint;
    return shaderStage;
}
