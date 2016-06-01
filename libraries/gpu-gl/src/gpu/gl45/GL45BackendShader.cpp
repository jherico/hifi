//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL45Backend.h"
#include "../gl/GLShader.h"

using namespace gpu;
using namespace gpu::gl45;

class GL45Shader : public gl::GLShader {
    using Parent = gpu::gl::GLShader;

public:
    GL45Shader(const Shader& Shader) : Parent(Shader) { }

    GLShader* buildChildShader(const Shader& child) override {
        return GL45Shader::sync<GL45Shader>(child);
    }

    std::string getShaderHeader(Version version) const override {
        // GLSL version
        const std::string glslVersion { "#version 450 core" };
        std::string shaderDefines = glslVersion + "\n" + DOMAIN_DEFINES[_gpuObject.getType()] + "\n" + VERSION_DEFINES[version];
        return shaderDefines;
    }

    void makeProgramBindings(ShaderObject& shaderObject) override {
        Parent::makeProgramBindings(shaderObject);

        const auto& glprogram = shaderObject.glprogram;

        //Check for gpu specific uniform slotBindings
        GLint loc = glGetProgramResourceIndex(glprogram, GL_SHADER_STORAGE_BLOCK, "transformObjectBuffer");
        if (loc >= 0) {
            glShaderStorageBlockBinding(glprogram, loc, TRANSFORM_OBJECT_SLOT);
            shaderObject.transformObjectSlot = TRANSFORM_OBJECT_SLOT;
        }

        loc = glGetUniformBlockIndex(glprogram, "transformCameraBuffer");
        if (loc >= 0) {
            glUniformBlockBinding(glprogram, loc, gpu::TRANSFORM_CAMERA_SLOT);
            shaderObject.transformCameraSlot = gpu::TRANSFORM_CAMERA_SLOT;
        }
    }
};

bool GL45Backend::makeProgram(Shader& shader, const Shader::BindingSet& bindings) {
    return GL45Shader::makeProgram<GL45Shader>(shader, bindings);
}

