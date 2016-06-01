//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLShader_h
#define hifi_gpu_gl_GLShader_h

#include "GLShared.h"

namespace gpu { namespace gl {

class GLShader : public GLObject<Shader> {
public:
    enum Version {
        Mono = 0,
        NumVersions,
        FirstVersion = Mono
    };

    using ShaderObject = gpu::gl::ShaderObject;
    using ShaderObjects = std::array< ShaderObject, NumVersions >;

    using UniformMapping = std::map<GLint, GLint>;
    using UniformMappingVersions = std::vector<UniformMapping>;

    static const size_t NUM_SHADER_DOMAINS = 2;
    static const std::array<std::string, NUM_SHADER_DOMAINS> DOMAIN_DEFINES;
    static const std::array<GLenum, NUM_SHADER_DOMAINS> SHADER_DOMAINS;
    static const std::array<std::string, NumVersions> VERSION_DEFINES;


    template <typename GLShaderType>
    static GLShaderType* sync(const Shader& shader) {
        GLShaderType* object = Backend::getGPUObject<GLShaderType>(shader);

        // If GPU object already created then good
        if (object) {
            return object;
        }

        auto result = new GLShaderType(shader);
        if (!result->compile()) {
            delete result;
            return nullptr;
        }
        return result;
    }

    template <typename GLShaderType>
    static bool makeProgram(Shader& shader, const Shader::BindingSet& slotBindings) {
        // First make sure the Shader has been compiled
        GLShaderType* object = sync<GLShaderType>(shader);
        if (!object) {
            return false;
        }


        // Apply bindings to all program versions and generate list of slots from default version
        for (int version = 0; version < GLShader::NumVersions; version++) {
            auto& shaderObject = object->_shaderObjects[version];
            if (shaderObject.glprogram) {
                Shader::SlotSet buffers;
                makeUniformBlockSlots(shaderObject.glprogram, slotBindings, buffers);

                Shader::SlotSet uniforms;
                Shader::SlotSet textures;
                Shader::SlotSet samplers;
                makeUniformSlots(shaderObject.glprogram, slotBindings, uniforms, textures, samplers);

                Shader::SlotSet inputs;
                makeInputSlots(shaderObject.glprogram, slotBindings, inputs);

                Shader::SlotSet outputs;
                makeOutputSlots(shaderObject.glprogram, slotBindings, outputs);

                // Define the public slots only from the default version
                if (version == 0) {
                    shader.defineSlots(uniforms, buffers, textures, samplers, inputs, outputs);
                } else {
                    GLShader::UniformMapping mapping;
                    for (auto srcUniform : shader.getUniforms()) {
                        mapping[srcUniform._location] = uniforms.findLocation(srcUniform._name);
                    }
                    object->_uniformMappings.push_back(mapping);
                }
            }
        }

        return true;
    }

    GLShader(const Shader& gpuObject);
    ~GLShader();

    ShaderObjects _shaderObjects;
    UniformMappingVersions _uniformMappings;

    GLuint getProgram(Version version = Mono) const {
        return _shaderObjects[version].glprogram;
    }

    GLint getUniformLocation(GLint srcLoc, Version version = Mono) {
        // THIS will be used in the future PR as we grow the number of versions
        // return _uniformMappings[version][srcLoc];
        return srcLoc;
    }

protected:
    virtual GLShader* buildChildShader(const Shader& child) = 0;
    virtual std::string getShaderHeader(Version version) const = 0;
    bool compile();
    bool compileBackendShader();
    bool compileBackendProgram();
    bool compileShader(Version version);
    bool compileProgram(Version version);
    void makeProgramBindings();
    virtual void makeProgramBindings(ShaderObject& shaderObject);

private:
    static int makeUniformSlots(GLuint glprogram, const Shader::BindingSet& slotBindings,
        Shader::SlotSet& uniforms, Shader::SlotSet& textures, Shader::SlotSet& samplers);
    static int makeUniformBlockSlots(GLuint glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& buffers);
    static int makeInputSlots(GLuint glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& inputs);
    static int makeOutputSlots(GLuint glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& outputs);

};



} }


#endif
