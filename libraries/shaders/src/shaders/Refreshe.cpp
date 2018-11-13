//
//  Created by Bradley Austin Davis on 2018/11/13
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Shaders.h"

#include <QtCore/QDebug>

#include <mutex>

#include <spirv_cross/spirv_cross.hpp>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>

using namespace shader;

static const TBuiltInResource* getGlslCompilerResources() {
    static TBuiltInResource glslCompilerResources;
    static std::once_flag once;
    std::call_once(once, [&] {
        glslCompilerResources.maxLights = 32;
        glslCompilerResources.maxClipPlanes = 6;
        glslCompilerResources.maxTextureUnits = 32;
        glslCompilerResources.maxTextureCoords = 32;
        glslCompilerResources.maxVertexAttribs = 64;
        glslCompilerResources.maxVertexUniformComponents = 4096;
        glslCompilerResources.maxVaryingFloats = 64;
        glslCompilerResources.maxVertexTextureImageUnits = 32;
        glslCompilerResources.maxCombinedTextureImageUnits = 80;
        glslCompilerResources.maxTextureImageUnits = 32;
        glslCompilerResources.maxFragmentUniformComponents = 4096;
        glslCompilerResources.maxDrawBuffers = 32;
        glslCompilerResources.maxVertexUniformVectors = 128;
        glslCompilerResources.maxVaryingVectors = 8;
        glslCompilerResources.maxFragmentUniformVectors = 16;
        glslCompilerResources.maxVertexOutputVectors = 16;
        glslCompilerResources.maxFragmentInputVectors = 15;
        glslCompilerResources.minProgramTexelOffset = -8;
        glslCompilerResources.maxProgramTexelOffset = 7;
        glslCompilerResources.maxClipDistances = 8;
        glslCompilerResources.maxComputeWorkGroupCountX = 65535;
        glslCompilerResources.maxComputeWorkGroupCountY = 65535;
        glslCompilerResources.maxComputeWorkGroupCountZ = 65535;
        glslCompilerResources.maxComputeWorkGroupSizeX = 1024;
        glslCompilerResources.maxComputeWorkGroupSizeY = 1024;
        glslCompilerResources.maxComputeWorkGroupSizeZ = 64;
        glslCompilerResources.maxComputeUniformComponents = 1024;
        glslCompilerResources.maxComputeTextureImageUnits = 16;
        glslCompilerResources.maxComputeImageUniforms = 8;
        glslCompilerResources.maxComputeAtomicCounters = 8;
        glslCompilerResources.maxComputeAtomicCounterBuffers = 1;
        glslCompilerResources.maxVaryingComponents = 60;
        glslCompilerResources.maxVertexOutputComponents = 64;
        glslCompilerResources.maxGeometryInputComponents = 64;
        glslCompilerResources.maxGeometryOutputComponents = 128;
        glslCompilerResources.maxFragmentInputComponents = 128;
        glslCompilerResources.maxImageUnits = 8;
        glslCompilerResources.maxCombinedImageUnitsAndFragmentOutputs = 8;
        glslCompilerResources.maxCombinedShaderOutputResources = 8;
        glslCompilerResources.maxImageSamples = 0;
        glslCompilerResources.maxVertexImageUniforms = 0;
        glslCompilerResources.maxTessControlImageUniforms = 0;
        glslCompilerResources.maxTessEvaluationImageUniforms = 0;
        glslCompilerResources.maxGeometryImageUniforms = 0;
        glslCompilerResources.maxFragmentImageUniforms = 8;
        glslCompilerResources.maxCombinedImageUniforms = 8;
        glslCompilerResources.maxGeometryTextureImageUnits = 16;
        glslCompilerResources.maxGeometryOutputVertices = 256;
        glslCompilerResources.maxGeometryTotalOutputComponents = 1024;
        glslCompilerResources.maxGeometryUniformComponents = 1024;
        glslCompilerResources.maxGeometryVaryingComponents = 64;
        glslCompilerResources.maxTessControlInputComponents = 128;
        glslCompilerResources.maxTessControlOutputComponents = 128;
        glslCompilerResources.maxTessControlTextureImageUnits = 16;
        glslCompilerResources.maxTessControlUniformComponents = 1024;
        glslCompilerResources.maxTessControlTotalOutputComponents = 4096;
        glslCompilerResources.maxTessEvaluationInputComponents = 128;
        glslCompilerResources.maxTessEvaluationOutputComponents = 128;
        glslCompilerResources.maxTessEvaluationTextureImageUnits = 16;
        glslCompilerResources.maxTessEvaluationUniformComponents = 1024;
        glslCompilerResources.maxTessPatchComponents = 120;
        glslCompilerResources.maxPatchVertices = 32;
        glslCompilerResources.maxTessGenLevel = 64;
        glslCompilerResources.maxViewports = 16;
        glslCompilerResources.maxVertexAtomicCounters = 0;
        glslCompilerResources.maxTessControlAtomicCounters = 0;
        glslCompilerResources.maxTessEvaluationAtomicCounters = 0;
        glslCompilerResources.maxGeometryAtomicCounters = 0;
        glslCompilerResources.maxFragmentAtomicCounters = 8;
        glslCompilerResources.maxCombinedAtomicCounters = 8;
        glslCompilerResources.maxAtomicCounterBindings = 1;
        glslCompilerResources.maxVertexAtomicCounterBuffers = 0;
        glslCompilerResources.maxTessControlAtomicCounterBuffers = 0;
        glslCompilerResources.maxTessEvaluationAtomicCounterBuffers = 0;
        glslCompilerResources.maxGeometryAtomicCounterBuffers = 0;
        glslCompilerResources.maxFragmentAtomicCounterBuffers = 1;
        glslCompilerResources.maxCombinedAtomicCounterBuffers = 1;
        glslCompilerResources.maxAtomicCounterBufferSize = 16384;
        glslCompilerResources.maxTransformFeedbackBuffers = 4;
        glslCompilerResources.maxTransformFeedbackInterleavedComponents = 64;
        glslCompilerResources.maxCullDistances = 8;
        glslCompilerResources.maxCombinedClipAndCullDistances = 8;
        glslCompilerResources.maxSamples = 4;
        glslCompilerResources.limits.nonInductiveForLoops = 1;
        glslCompilerResources.limits.whileLoops = 1;
        glslCompilerResources.limits.doWhileLoops = 1;
        glslCompilerResources.limits.generalUniformIndexing = 1;
        glslCompilerResources.limits.generalAttributeMatrixVectorIndexing = 1;
        glslCompilerResources.limits.generalVaryingIndexing = 1;
        glslCompilerResources.limits.generalSamplerIndexing = 1;
        glslCompilerResources.limits.generalVariableIndexing = 1;
        glslCompilerResources.limits.generalConstantMatrixVectorIndexing = 1;
    });
    return &glslCompilerResources;
}

void convertSpirvToBytes(const std::vector<uint32_t>& inSpirv, std::vector<uint8_t>& outSpirv){
    auto size = inSpirv.size() * sizeof(uint32_t);
    if (size > 0) {
        outSpirv.resize(size);
        memcpy(outSpirv.data(), inSpirv.data(), size);
    }
}


Source::Pointer Source::generate(const std::string& glsl, Stage shaderStage, const std::string& entryPoint) {
    try {
        static std::once_flag once;
        std::call_once(once, [&] {
            glslang::InitializeProcess();
            //glslang::FinalizeProcess();
        });

        DialectVariantSource result;
        result.scribe = glsl;
        using namespace glslang;
        static const EShMessages messages = (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);
        auto stage = (EShLanguage)shaderStage;
        TShader shader(stage);
        std::vector<const char*> strings;
        strings.push_back(glsl.c_str());
        shader.setStrings(strings.data(), (int)strings.size());
        shader.setEnvInput(EShSourceGlsl, stage, EShClientOpenGL, 450);
        shader.setEnvClient(EShClientVulkan, EShTargetVulkan_1_1);
        shader.setEnvTarget(EShTargetSpv, EShTargetSpv_1_3);

        bool success = shader.parse(getGlslCompilerResources(), 450, false, messages);
        if (!success) {
            qWarning() << "Failed to parse shader";
            qWarning() << shader.getInfoLog();
            qWarning() << shader.getInfoDebugLog();
            return nullptr;
        }

        // Create and link a shader program containing the single shader
        glslang::TProgram program;
        program.addShader(&shader);
        if (!program.link(messages)) {
            qWarning() << "Failed to compile shader";
            qWarning() << program.getInfoLog();
            qWarning() << program.getInfoDebugLog();
            return nullptr;
        }

        // Output the SPIR-V code from the shader program
        std::vector<uint32_t> spirv;
        glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);


        spvtools::SpirvTools core(SPV_ENV_VULKAN_1_1);
        spvtools::Optimizer opt(SPV_ENV_VULKAN_1_1);

        auto outputLambda = [](spv_message_level_t, const char*, const spv_position_t&, const char* m) { qWarning() << m; };
        core.SetMessageConsumer(outputLambda);
        opt.SetMessageConsumer(outputLambda);
        if (!core.Validate(spirv)) {
            throw std::runtime_error("invalid spirv");
        }

        opt.RegisterPass(spvtools::CreateFreezeSpecConstantValuePass())
            .RegisterPass(spvtools::CreateStrengthReductionPass())
            .RegisterPass(spvtools::CreateEliminateDeadConstantPass())
            .RegisterPass(spvtools::CreateEliminateDeadFunctionsPass())
            .RegisterPass(spvtools::CreateUnifyConstantPass());

        std::vector<uint32_t> optspirv;
        if (!opt.Run(spirv.data(), spirv.size(), &optspirv)) {
            throw std::runtime_error("bad optimize run");
        }
        convertSpirvToBytes(optspirv, result.spirv);
    } catch (const std::runtime_error& error) {
        qWarning() << error.what();
        return nullptr;
    }
    return nullptr;
}
