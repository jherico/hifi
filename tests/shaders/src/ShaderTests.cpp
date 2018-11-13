//
//  ShaderTests.cpp
//  tests/octree/src
//
//  Created by Andrew Meadows on 2016.02.19
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ShaderTests.h"

#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>

#include <QtCore/QJsonDocument>
#include <QtConcurrent/QtConcurrent>

#include <GLMHelpers.h>

#include <test-utils/GLMTestUtils.h>
#include <test-utils/QTestExtensions.h>
#include <shared/FileUtils.h>

#include <shaders/Shaders.h>
#include <gl/Config.h>
#include <gl/GLHelpers.h>
#include <gpu/gl/GLBackend.h>
#include <gpu/gl/GLShader.h>
#include <gl/QOpenGLContextWrapper.h>


//#include <vulkan/vulkan.hpp>

QTEST_MAIN(ShaderTests)

void ShaderTests::initTestCase() {
    _context = new ::gl::OffscreenContext();
    getDefaultOpenGLSurfaceFormat();
    _context->create();
    if (!_context->makeCurrent()) {
        qFatal("Unable to make test GL context current");
    }
    QOpenGLContextWrapper(_context->qglContext()).makeCurrent(_context->getWindow());
    gl::initModuleGl();
    if (!_context->makeCurrent()) {
        qFatal("Unable to make test GL context current");
    }
    gpu::Context::init<gpu::gl::GLBackend>();
    if (!_context->makeCurrent()) {
        qFatal("Unable to make test GL context current");
    }
    _gpuContext = std::make_shared<gpu::Context>();
}

void ShaderTests::cleanupTestCase() {
    qDebug() << "Done";
}

#if 0
gpu::Shader::ReflectionMap mergeReflection(const std::initializer_list<const gpu::Shader::Source>& list) {
    gpu::Shader::ReflectionMap result;
    std::unordered_map<gpu::Shader::BindingType, std::unordered_map<uint32_t, std::string>> usedLocationsByType;
    for (const auto& source : list) {
        const auto& reflection = source.getReflection();
        for (const auto& entry : reflection) {
            const auto& type = entry.first;
            if (entry.first == gpu::Shader::BindingType::INPUT || entry.first == gpu::Shader::BindingType::OUTPUT) {
                continue;
            }
            auto& outLocationMap = result[type];
            auto& usedLocations = usedLocationsByType[type];
            const auto& locationMap = entry.second;
            for (const auto& entry : locationMap) {
                const auto& name = entry.first;
                const auto& location = entry.second;
                if (0 != usedLocations.count(location) && usedLocations[location] != name) {
                    qWarning() << QString("Location %1 used by both %2 and %3")
                                      .arg(location)
                                      .arg(name.c_str())
                                      .arg(usedLocations[location].c_str());
                    throw std::runtime_error("Location collision");
                }
                usedLocations[location] = name;
                outLocationMap[name] = location;
            }
        }
    }
    return result;
}

template <typename C>
QStringList toQStringList(const C& c) {
    QStringList result;
    for (const auto& v : c) {
        result << v.c_str();
    }
    return result;
}

template <typename C>
bool isSubset(const C& parent, const C& child) {
    for (const auto& v : child) {
        if (0 == parent.count(v)) {
            return false;
        }
    }
    return true;
}

template <typename C, typename F>
std::unordered_set<std::string> toStringSet(const C& c, F f) {
    std::unordered_set<std::string> result;
    for (const auto& v : c) {
        result.insert(f(v));
    }
    return result;
}

template <typename C>
bool compareBindings(const C& actual, const gpu::Shader::LocationMap& expected) {
    if (actual.size() != expected.size()) {
        auto actualNames = toStringSet(actual, [](const auto& v) { return v.name; });
        auto expectedNames = toStringSet(expected, [](const auto& v) { return v.first; });
        if (!isSubset(expectedNames, actualNames)) {
            qDebug() << "Found" << toQStringList(actualNames);
            qDebug() << "Expected" << toQStringList(expectedNames);
            return false;
        }
    }
    return true;
}

#endif

template <typename K, typename V>
std::unordered_map<V, K> invertMap(const std::unordered_map<K, V>& map) {
    std::unordered_map<V, K> result;
    for (const auto& entry : map) {
        result[entry.second] = entry.first;
    }
    if (result.size() != map.size()) {
        throw std::runtime_error("Map inversion failure, result size does not match input size");
    }
    return result;
}

static void verifyInterface(const gpu::Shader::Source& vertexSource,
                            const gpu::Shader::Source& fragmentSource,
                            shader::Dialect dialect,
                            shader::Variant variant) {
    const auto& fragmentReflection = fragmentSource.getReflection(dialect, variant);
    if (fragmentReflection.inputs.empty()) {
        return;
    }

    const auto& vertexReflection = vertexSource.getReflection(dialect, variant);
    const auto& fragIn = fragmentReflection.inputs;
    if (vertexReflection.outputs.empty()) {
        throw std::runtime_error("No vertex outputs for fragment inputs");
    }

    const auto& vout = vertexReflection.outputs;
    auto vrev = invertMap(vout);
    for (const auto entry : fragIn) {
        const auto& name = entry.first;
        if (0 == vout.count(name)) {
            throw std::runtime_error("Vertex outputs missing");
        }
        const auto& inLocation = entry.second;
        const auto& outLocation = vout.at(name);
        if (inLocation != outLocation) {
            throw std::runtime_error("Mismatch in vertex / fragment interface");
        }
    }
}

static void verifyInterface(const gpu::Shader::Source& vertexSource, const gpu::Shader::Source& fragmentSource) {
    for (const auto& dialect : shader::allDialects()) {
        for (const auto& variant : shader::allVariants()) {
            verifyInterface(vertexSource, fragmentSource, dialect, variant);
        }
    }
}

bool endsWith(const std::string& s, const std::string& f) {
    auto end = s.substr(s.size() - f.size());
    return (end == f);
}

shader::Stage getShaderStage(const std::string& shaderName) {
    static const std::string VERT_EXT{ ".vert" };
    static const std::string FRAG_EXT{ ".frag" };
    static const std::string GEOM_EXT{ ".geom" };
    static const size_t EXT_SIZE = VERT_EXT.size();
    if (shaderName.size() < EXT_SIZE) {
        throw std::runtime_error("Invalid shader name");
    }
    std::string ext = shaderName.substr(shaderName.size() - EXT_SIZE);
    if (ext == VERT_EXT) {
        return shader::Stage::Vertex;
    } else if (ext == FRAG_EXT) {
        return shader::Stage::Fragment;
    } else if (ext == GEOM_EXT) {
        return shader::Stage::Geometry;
    }
    throw std::runtime_error("Invalid shader name");
}

void rebuildSource(shader::Dialect dialect, shader::Variant variant, const shader::Source& source) {
    try {
        auto stage = getShaderStage(source.name);
        const auto& dialectVariantSource = source.dialectSources.at(dialect).variantSources.at(variant);
        auto rebuilt = shader::Source::generate(dialectVariantSource.scribe, stage);
    } catch (const std::runtime_error& error) {
        qWarning() << error.what();
    }
}

uint32_t makeProgramId(uint32_t vertexId, uint32_t fragmentId) {
    return (vertexId << 16) | fragmentId;
}

std::unordered_map<uint32_t, std::list<uint32_t>> shaderToProgramMap;
std::unordered_map<uint32_t, std::unordered_set<uint32_t>> problemShaderIds;

void validateDialectVariantSource(const shader::DialectVariantSource& source) {
    if (source.scribe.empty()) {
        throw std::runtime_error("Missing scribe source");
    }

    if (source.spirv.empty()) {
        throw std::runtime_error("Missing SPIRV");
    }

    if (source.glsl.empty()) {
        throw std::runtime_error("Missing GLSL");
    }
}

void validaDialectSource(const shader::DialectSource& dialectSource) {
    for (const auto& variant : shader::allVariants()) {
        validateDialectVariantSource(dialectSource.variantSources.find(variant)->second);
    }
}

void validateSource(const shader::Source& shader) {
    if (shader.id == shader::INVALID_SHADER) {
        throw std::runtime_error("Missing stored shader ID");
    }

    if (shader.name.empty()) {
        throw std::runtime_error("Missing shader name");
    }

    static const auto& dialects = shader::allDialects();
    for (const auto dialect : dialects) {
        if (!shader.dialectSources.count(dialect)) {
            throw std::runtime_error("Missing platform shader");
        }
        const auto& platformShader = shader.dialectSources.find(dialect)->second;
        validaDialectSource(platformShader);
    }
}

void ShaderTests::testShaderLoad() {
    try {
        const auto& shaderIds = shader::allShaders();

        // For debugging compile or link failures on individual programs, enable the following block and change the array values
        // Be sure to end with the sentinal value of shader::INVALID_PROGRAM
        const auto& programIds = shader::allPrograms();

        for (auto shaderId : shaderIds) {
            validateSource(shader::Source::get(shaderId));
        }

        {
            std::unordered_set<uint32_t> programUsedShaders;
#pragma omp parallel for
            for (auto programId : programIds) {
                auto vertexId = shader::getVertexId(programId);
                shader::Source::get(vertexId);
                programUsedShaders.insert(vertexId);
                auto fragmentId = shader::getFragmentId(programId);
                shader::Source::get(fragmentId);
                programUsedShaders.insert(fragmentId);
            }

            for (const auto& shaderId : shaderIds) {
                if (programUsedShaders.count(shaderId)) {
                    continue;
                }
                const auto& shader = shader::Source::get(shaderId);
                qDebug() << "Unused shader found" << shader.name.c_str();
            }

            //auto threadPool = QThreadPool::globalInstance();
            //threadPool->setMaxThreadCount(24);
            for (const auto& shaderId : programUsedShaders) {
                const auto& shaderSource = shader::Source::get(shaderId);
                for (const auto& dialect : shader::allDialects()) {
                    for (const auto& variant : shader::allVariants()) {
                        rebuildSource(dialect, variant, shaderSource);
                    }
                }
            }
        }


        // Traverse all programs again to do program level tests
        for (const auto& programId : programIds) {
            auto vertexId = shader::getVertexId(programId);
            const auto& vertexSource = shader::Source::get(vertexId);
            auto fragmentId = shader::getFragmentId(programId);
            const auto& fragmentSource = shader::Source::get(fragmentId);
            //verifyInterface(vertexSource, fragmentSource);

            //continue;

            auto program = gpu::Shader::createProgram(programId);
            auto glBackend = std::static_pointer_cast<gpu::gl::GLBackend>(_gpuContext->getBackend());
            auto glshader = gpu::gl::GLShader::sync(*glBackend, *program);

            if (!glshader) {
                qWarning() << "Failed to compile or link vertex " << vertexSource.name.c_str() << " fragment "
                           << fragmentSource.name.c_str();
                QFAIL("Program link error");
            }
        }

        for (const auto& entry : problemShaderIds) {
            const auto& shaderId = entry.first;
            const auto& otherShadersSet = entry.second;
            auto name = shader::Source::get(shaderId).name;
            for (const auto& programId : shaderToProgramMap[shaderId]) {
                auto vertexId = (programId >> 16) & 0xFFFF;
                auto fragmentId = programId & 0xFFFF;
                auto otherShaderId = shaderId == vertexId ? fragmentId : vertexId;
                if (otherShadersSet.count(otherShaderId) != 0) {
                    continue;
                }
                auto otherName = shader::Source::get(otherShaderId).name;
                qWarning() << "Shader id " << name.c_str() << " used with shader " << otherName.c_str();
            }
        }
    } catch (const std::runtime_error& error) {
        QFAIL(error.what());
    }

    qDebug() << "Completed all shaders";
}
