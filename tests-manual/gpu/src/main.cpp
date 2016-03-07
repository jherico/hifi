//
//  main.cpp
//  tests/gpu-test/src
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <unordered_map>
#include <memory>
#include <cstdio>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>

#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>

#include <gpu/Context.h>
#include <gpu/Batch.h>
#include <gpu/Stream.h>
#include <gpu/gl/GLBackend.h>

#include <gl/QOpenGLContextWrapper.h>
#include <gl/GLHelpers.h>
#include <gl/GLWindow.h>

#include <GLMHelpers.h>
#include <PathUtils.h>
#include <NumericalConstants.h>

#include <GeometryCache.h>
#include <DeferredLightingEffect.h>
#include <FramebufferCache.h>
#include <TextureCache.h>
#include <PerfStat.h>
#include <PathUtils.h>
#include <SharedUtil.h>
#include <ViewFrustum.h>

#include <gpu/Pipeline.h>
#include <gpu/Context.h>

#include <render/Engine.h>
#include <render/Scene.h>
#include <render/CullTask.h>
#include <render/SortTask.h>
#include <render/DrawTask.h>
#include <render/DrawStatus.h>
#include <render/DrawSceneOctree.h>
#include <render/CullTask.h>

#include "TestWindow.h"
#include "TestFbx.h"
#include "TestFloorGrid.h"
#include "TestFloorTexture.h"
#include "TestInstancedShapes.h"
#include "TestShapes.h"

// assimp include files. These three are usually needed.
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>


const aiScene* scene { nullptr };
std::map<std::string, GLuint*> textureIdMap;	// map image filenames to textureIds


std::map<std::string, FBXMaterial> materials;


//
//
//class FBXMaterial {
//public:
//    FBXMaterial() {};
//    FBXMaterial(const glm::vec3& diffuseColor, const glm::vec3& specularColor, const glm::vec3& emissiveColor,
//        float shininess, float opacity) :
//        diffuseColor(diffuseColor),
//        specularColor(specularColor),
//        emissiveColor(emissiveColor),
//        shininess(shininess),
//        opacity(opacity) {
//    }
//
//    glm::vec3 diffuseColor { 1.0f };
//    float diffuseFactor = 1.0f;
//    glm::vec3 specularColor { 0.02f };
//    float specularFactor = 1.0f;
//
//    glm::vec3 emissiveColor { 0.0f };
//    float shininess = 23.0f;
//    float opacity = 1.0f;
//
//    float metallic { 0.0f };
//    float roughness { 1.0f };
//    float emissiveIntensity { 1.0f };
//
//    QString materialID;
//    model::MaterialPointer _material;
//
//    FBXTexture normalTexture;
//    FBXTexture albedoTexture;
//    FBXTexture opacityTexture;
//    FBXTexture glossTexture;
//    FBXTexture roughnessTexture;
//    FBXTexture specularTexture;
//    FBXTexture metallicTexture;
//    FBXTexture emissiveTexture;
//    FBXTexture occlusionTexture;
//    FBXTexture lightmapTexture;
//    glm::vec2 lightmapParams { 0.0f, 1.0f };
//
//
//    bool isPBSMaterial { false };
//    // THe use XXXMap are not really used to drive which map are going or not, debug only
//    bool useNormalMap { false };
//    bool useAlbedoMap { false };
//    bool useOpacityMap { false };
//    bool useRoughnessMap { false };
//    bool useSpecularMap { false };
//    bool useMetallicMap { false };
//    bool useEmissiveMap { false };
//    bool useOcclusionMap { false };
//
//    bool needTangentSpace() const;
//};

const char* TEST_FILE = "C:/Users/bdavis/Git/dreaming/assets/models/mjolnir-ucac5/Mjolnir Test 3.obj";

void loadScene() {
    static Assimp::Importer importer;
    scene = importer.ReadFile(TEST_FILE, aiProcessPreset_TargetRealtime_MaxQuality);

    // If the import failed, report it
    if (!scene) {
        qFatal(importer.GetErrorString());
    }

    if (scene->HasTextures()) {
        qFatal("Support for meshes with embedded textures is not implemented");
    }


    //aiTextureType_DIFFUSE = 0x1,
    //aiTextureType_SPECULAR = 0x2,
    //aiTextureType_AMBIENT = 0x3,
    //aiTextureType_EMISSIVE = 0x4,
    //aiTextureType_HEIGHT = 0x5,
    //aiTextureType_NORMALS = 0x6,
    //aiTextureType_SHININESS = 0x7,
    //aiTextureType_OPACITY = 0x8,
    //aiTextureType_DISPLACEMENT = 0x9,
    //aiTextureType_LIGHTMAP = 0xA,
    //aiTextureType_REFLECTION = 0xB,

    
    for (unsigned int m = 0; m<scene->mNumMaterials; m++) {
        FBXMaterial material;
        std::string materialName;
        const auto& mat = scene->mMaterials[m];
        aiString name;
        if (AI_SUCCESS == aiGetMaterialString(mat, AI_MATKEY_NAME, &name)) {
            qDebug() << "name: " << name.data;
            material.materialID = name.data;
        }
        if (AI_SUCCESS == aiGetMaterialString(mat, AI_MATKEY_SHADING_MODEL, &name)) {
            qDebug() << "shading model: " << name.data;
        }
        aiColor4D color;
        if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &color)) {
            qDebug() << "diffuse: " << color.r << " " << color.g << " " << color.b << " " << color.a;
        }
        if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_SPECULAR, &color)) {
            qDebug() << "specular: " << color.r << " " << color.g << " " << color.b << " " << color.a;
        }
        if (AI_SUCCESS == aiGetMaterialColor(mat, AI_MATKEY_COLOR_EMISSIVE, &color)) {
            qDebug() << "emissive: " << color.r << " " << color.g << " " << color.b << " " << color.a;
        }
        float f;
        if (AI_SUCCESS == aiGetMaterialFloat(mat, AI_MATKEY_SHININESS_STRENGTH, &f)) {
            qDebug() << "shininess: " << f;
        }
        if (AI_SUCCESS == aiGetMaterialFloat(mat, AI_MATKEY_OPACITY, &f)) {
            qDebug() << "opacity: " << f;
        }
        int texIndex = 0;
        aiReturn texFound = AI_SUCCESS;
        aiString path;	// filename
        while (texFound == AI_SUCCESS) {
            texFound = mat->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
            textureIdMap[path.data] = NULL; //fill map with textures, pointers still NULL yet
            texIndex++;
        }
    }

}


gpu::ShaderPointer makeShader(const std::string & vertexShaderSrc, const std::string & fragmentShaderSrc, const gpu::Shader::BindingSet & bindings) {
    auto vs = gpu::Shader::createVertex(vertexShaderSrc);
    auto fs = gpu::Shader::createPixel(fragmentShaderSrc);
    auto shader = gpu::Shader::createProgram(vs, fs);
    if (!gpu::Shader::makeProgram(*shader, bindings)) {
        printf("Could not compile shader/n");
        exit(-1);
    }
    return shader;
}


using TestBuilder = std::function<GpuTestBase*()>;
using TestBuilders = std::list<TestBuilder>;


// Creates an OpenGL window that renders a simple unlit scene using the gpu library and GeometryCache
// Should eventually get refactored into something that supports multiple gpu backends.
class QTestWindow : public GLWindow {
    Q_OBJECT

    gpu::ContextPointer _context;
    gpu::PipelinePointer _pipeline;
    glm::mat4 _projectionMatrix;
    RateCounter fps;
    QTime _time;
    gpu::BufferPointer _testVertices;

public:
    QTestWindow() {
        createContext();
        show();
        makeCurrent();
        setupDebugLogger(this);

        loadScene();

        gpu::Context::init<gpu::GLBackend>();
        _context = std::make_shared<gpu::Context>();
        
        auto shader = makeShader(unlit_vert, unlit_frag, gpu::Shader::BindingSet{});
        auto state = std::make_shared<gpu::State>();
        state->setMultisampleEnable(true);
        state->setDepthTest(gpu::State::DepthTest { true });
        _pipeline = gpu::Pipeline::create(shader, state);



        // Clear screen
        gpu::Batch batch;
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLORS, { 1.0, 0.0, 0.5, 1.0 });
        _context->render(batch);

        _testVertices = std::make_shared<gpu::Buffer>();

        DependencyManager::set<GeometryCache>();
        DependencyManager::set<DeferredLightingEffect>();

        resize(QSize(800, 600));
        
        _time.start();
#endif
        updateCamera();
        _testBuilders = TestBuilders({
            [this] { return new TestFbx(_shapePlumber); },
            [] { return new TestInstancedShapes(); },
        });
    }

    virtual ~QTestWindow() {
    }

    void draw() {
        // Attempting to draw before we're visible and have a valid size will
        // produce GL errors.
        const auto _size = size();
        if (!isVisible() || _size.width() <= 0 || _size.height() <= 0) {
            return;
        }
        makeCurrent();
        
        gpu::Batch batch;
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLORS, { 0.0f, 0.0f, 0.0f, 1.0f });
        batch.clearDepthFramebuffer(1e4);
        batch.setViewportTransform({ 0, 0, _size.width() * devicePixelRatio(), _size.height() * devicePixelRatio() });
        batch.setProjectionTransform(_projectionMatrix);
        
        float t = _time.elapsed() * 1e-3f;
        glm::vec3 unitscale { 1.0f };
        glm::vec3 up { 0.0f, 1.0f, 0.0f };

        float distance = 3.0f;
        glm::vec3 camera_position { distance * sinf(t), 0.5f, distance * cosf(t) };

        static const vec3 camera_focus(0);
        static const vec3 camera_up(0, 1, 0);
        _camera = glm::inverse(glm::lookAt(camera_position, camera_focus, up));

        ViewFrustum frustum;
        frustum.setPosition(camera_position);
        frustum.setOrientation(glm::quat_cast(_camera));
        frustum.setProjection(_projectionMatrix);
        _renderArgs->setViewFrustum(frustum);
    }

    void renderFrame() override {
        updateCamera();

        while ((!_currentTest || (_currentTestId >= _currentMaxTests)) && !_testBuilders.empty()) {
            if (_currentTest) {
                delete _currentTest;
                _currentTest = nullptr;
            }

            _currentTest = _testBuilders.front()();
            _testBuilders.pop_front();

            if (_currentTest) {
                _currentMaxTests = _currentTest->getTestCount();
                _currentTestId = 0;
            }
        }

        if (!_currentTest && _testBuilders.empty()) {
            qApp->quit();
            return;
        }

        // Tests might need to wait for resources to download
        if (!_currentTest->isReady()) {
            return;
        }

        gpu::doInBatch("main::renderFrame", _renderArgs->_context, [&](gpu::Batch& batch) {
            batch.setViewTransform(_camera);
            _renderArgs->_batch = &batch;
            _currentTest->renderTest(_currentTestId, _renderArgs);
            _renderArgs->_batch = nullptr;
        });

#ifdef INTERACTIVE

        if (wire) {
            geometryCache->renderWireShape(batch, SHAPE[shapeIndex]);
        } else {
            geometryCache->renderShape(batch, SHAPE[shapeIndex]);
        }
        
        batch.setModelTransform(Transform().setScale(2.05f));
        batch._glColor4f(1, 1, 1, 1);
        geometryCache->renderWireCube(batch);

        _context->render(batch);
        swapBuffers();
        
        fps.increment();
        if (fps.elapsed() >= 0.5f) {
            qDebug() << "FPS: " << fps.rate();
            fps.reset();
        }
    }
    
protected:
    void resizeEvent(QResizeEvent* ev) override {
        auto _size = ev->size();
        float fov_degrees = 60.0f;
        float aspect_ratio = (float)_size.width() / _size.height();
        float near_clip = 0.1f;
        float far_clip = 1000.0f;
        _projectionMatrix = glm::perspective(glm::radians(fov_degrees), aspect_ratio, near_clip, far_clip);
    }
};

extern uvec2 rectifySize(const uvec2& size);

void testSparseRectify() {
    std::vector<std::pair<uvec2, uvec2>> SPARSE_SIZE_TESTS {
        // Already sparse
        { {1024, 1024 }, { 1024, 1024 } },
        { { 128, 128 }, { 128, 128 } },
        // Too small in one dimension
        { { 127, 127 }, { 128, 128 } },
        { { 1, 1 }, { 1, 1 } },
        { { 1000, 1 }, { 1024, 1 } },
        { { 1024, 1 }, { 1024, 1 } },
        { { 100, 100 }, { 128, 128 } },
        { { 57, 510 }, { 64, 512 } },
        // needs rectification
        { { 1000, 1000 }, { 1024, 1024 } },
        { { 1024, 1000 }, { 1024, 1024 } },
    };

    for (const auto& test : SPARSE_SIZE_TESTS) {
        const auto& size = test.first;
        const auto& expected = test.second;
        auto result = rectifySize(size);
        Q_ASSERT(expected == result);
        result = rectifySize(uvec2(size.y, size.x));
        Q_ASSERT(expected == uvec2(result.y, result.x));
    }
}

int main(int argc, char** argv) {
    setupHifiApplication("GPU Test");

    testSparseRectify();

    // FIXME this test appears to be broken
#if 0
    QGuiApplication app(argc, argv);
    MyTestWindow window;
    app.exec();
#endif
    return 0;
}

