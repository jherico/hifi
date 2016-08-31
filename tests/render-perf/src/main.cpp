//
//  Created by Bradley Austin Davis on 2016/07/01
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include <gl/Config.h>
#include <gl/Context.h>

#include <QProcessEnvironment>

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QThreadPool>

#include <QtGui/QGuiApplication>
#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>

#include <shared/RateCounter.h>
#include <shared/NetworkUtils.h>
#include <shared/FileLogger.h>
#include <shared/FileUtils.h>
#include <StatTracker.h>
#include <LogHandler.h>
#include <AssetClient.h>

#include <gpu/gl/GLBackend.h>
#include <gpu/gl/GLFramebuffer.h>
#include <gpu/gl/GLTexture.h>
#include <gpu/StandardShaderLib.h>

#include <SimpleEntitySimulation.h>
#include <EntityActionInterface.h>
#include <EntityActionFactoryInterface.h>
#include <display-plugins/DisplayPlugin.h>
#include <display-plugins/CompositorHelper.h>
#include <plugins/PluginManager.h>
#include <ui-plugins/PluginContainer.h>
#include <WebEntityItem.h>
#include <OctreeUtils.h>
#include <render/Engine.h>
#include <Model.h>
#include <model/Stage.h>
#include <TextureCache.h>
#include <FramebufferCache.h>
#include <model-networking/ModelCache.h>
#include <GeometryCache.h>
#include <DeferredLightingEffect.h>
#include <render/RenderFetchCullSortTask.h>
#include <RenderShadowTask.h>
#include <RenderDeferredTask.h>
#include <RenderForwardTask.h>
#include <OctreeConstants.h>
#include <MainWindow.h>
#include <gl/GLWidget.h>
#include <ui/Menu.h>
#include <EntityTreeRenderer.h>
#include <AbstractViewStateInterface.h>
#include <AddressManager.h>
#include <SceneScriptingInterface.h>

#include "Camera.hpp"

Q_DECLARE_LOGGING_CATEGORY(renderperflogging)
Q_LOGGING_CATEGORY(renderperflogging, "hifi.render_perf")

static const QString LAST_SCENE_KEY = "lastSceneFile";
static const QString LAST_LOCATION_KEY = "lastLocation";

class ParentFinder : public SpatialParentFinder {
public:
    EntityTreePointer _tree;
    ParentFinder(EntityTreePointer tree) : _tree(tree) {}

    SpatiallyNestableWeakPointer find(QUuid parentID, bool& success, SpatialParentTree* entityTree = nullptr) const override {
        SpatiallyNestableWeakPointer parent;

        if (parentID.isNull()) {
            success = true;
            return parent;
        }

        // search entities
        if (entityTree) {
            parent = entityTree->findByID(parentID);
        } else {
            parent = _tree ? _tree->findEntityByEntityItemID(parentID) : nullptr;
        }

        if (!parent.expired()) {
            success = true;
            return parent;
        }

        success = false;
        return parent;
    }
};

class QWindowCamera : public Camera {
    Key forKey(int key) {
        switch (key) {
            case Qt::Key_W: return FORWARD;
            case Qt::Key_S: return BACK;
            case Qt::Key_A: return LEFT;
            case Qt::Key_D: return RIGHT;
            case Qt::Key_E: return UP;
            case Qt::Key_C: return DOWN;
            default: break;
        }
        return INVALID;
    }

    vec2 _lastMouse;
public:
    void onKeyPress(QKeyEvent* event) {
        Key k = forKey(event->key());
        if (k == INVALID) {
            return;
        }
        keys.set(k);
    }

    void onKeyRelease(QKeyEvent* event) {
        Key k = forKey(event->key());
        if (k == INVALID) {
            return;
        }
        keys.reset(k);
    }

    void onMouseMove(QMouseEvent* event) {
        vec2 mouse = toGlm(event->localPos());
        vec2 delta = mouse - _lastMouse;
        auto buttons = event->buttons();
        if (buttons & Qt::RightButton) {
            dolly(delta.y * 0.01f);
        } else if (buttons & Qt::LeftButton) {
            //rotate(delta.x * -0.01f);
            rotate(delta * -0.01f);
        } else if (buttons & Qt::MiddleButton) {
            delta.y *= -1.0f;
            translate(delta * -0.01f);
        }

        _lastMouse = mouse;
    }
};

static QString toHumanSize(size_t size, size_t maxUnit = std::numeric_limits<size_t>::max()) {
    static const std::vector<QString> SUFFIXES { { "B", "KB", "MB", "GB", "TB", "PB" } };
    const size_t maxIndex = std::min(maxUnit, SUFFIXES.size() - 1);
    size_t suffixIndex = 0;

    while (suffixIndex < maxIndex && size > 1024) {
        size >>= 10;
        ++suffixIndex;
    }

    return QString("%1 %2").arg(size).arg(SUFFIXES[suffixIndex]);
}


class TestActionFactory : public EntityActionFactoryInterface {
public:
    virtual EntityActionPointer factory(EntityActionType type,
        const QUuid& id,
        EntityItemPointer ownerEntity,
        QVariantMap arguments) override {
        return EntityActionPointer();
    }


    virtual EntityActionPointer factoryBA(EntityItemPointer ownerEntity, QByteArray data) override {
        return nullptr;
    }
};

// Background Render Data & rendering functions
class BackgroundRenderData {
public:
    typedef render::Payload<BackgroundRenderData> Payload;
    typedef Payload::DataPointer Pointer;
    static render::ItemID _item; // unique WorldBoxRenderData
};

render::ItemID BackgroundRenderData::_item = 0;
QSharedPointer<FileLogger> logger;

namespace render {
    template <> const ItemKey payloadGetKey(const BackgroundRenderData::Pointer& stuff) {
        return ItemKey::Builder::background();
    }

    template <> const Item::Bound payloadGetBound(const BackgroundRenderData::Pointer& stuff) {
        return Item::Bound();
    }

    template <> void payloadRender(const BackgroundRenderData::Pointer& background, RenderArgs* args) {
        Q_ASSERT(args->_batch);
        gpu::Batch& batch = *args->_batch;

        // Background rendering decision
        auto skyStage = DependencyManager::get<SceneScriptingInterface>()->getSkyStage();
        auto backgroundMode = skyStage->getBackgroundMode();

        switch (backgroundMode) {
            case model::SunSkyStage::SKY_BOX: {
                auto skybox = skyStage->getSkybox();
                if (skybox) {
                    PerformanceTimer perfTimer("skybox");
                    skybox->render(batch, args->getViewFrustum());
                    break;
                }
            }
            default:
                // this line intentionally left blank
                break;
        }
    }
}

class GLTestApp : public QApplication, public PluginContainer {
public:
    //"/-17.2049,-8.08629,-19.4153/0,0.881994,0,-0.47126"
    static void setup() {
        DependencyManager::registerInheritance<EntityActionFactoryInterface, TestActionFactory>();
        DependencyManager::registerInheritance<LimitedNodeList, NodeList>();
        DependencyManager::registerInheritance<SpatialParentFinder, ParentFinder>();
        DependencyManager::set<tracing::Tracer>();
        DependencyManager::set<StatTracker>();
        DependencyManager::set<AddressManager>();
        DependencyManager::set<NodeList>(NodeType::Agent);
        DependencyManager::set<DeferredLightingEffect>();
        DependencyManager::set<ResourceCacheSharedItems>();
        DependencyManager::set<TextureCache>();
        DependencyManager::set<FramebufferCache>();
        DependencyManager::set<GeometryCache>();
        DependencyManager::set<ModelCache>();
        DependencyManager::set<AnimationCache>();
        DependencyManager::set<ModelBlender>();
        DependencyManager::set<PathUtils>();
        DependencyManager::set<SceneScriptingInterface>();
        DependencyManager::set<CompositorHelper>();
        DependencyManager::set<TestActionFactory>();
    }

    GLTestApp(int& argc, char** argv) : QApplication(argc, argv) {
        setApplicationName("RenderPerf");
        setOrganizationName("High Fidelity");
        setOrganizationDomain("highfidelity.com");
        setup();

        _window = new MainWindow(desktop());
        _glWidget = new GLWidget();
        DependencyManager::get<CompositorHelper>()->setRenderingWidget(_glWidget);
        _window->setCentralWidget(_glWidget);
        _window->restoreGeometry();
        _window->setVisible(true);
        _glWidget->setFocusPolicy(Qt::StrongFocus);
        _glWidget->setFocus();

        QCoreApplication::processEvents();
        _glWidget->createContext();
        _glWidget->makeCurrent();

        gpu::Context::init<gpu::gl::GLBackend>();
        _gpuContext = std::make_shared<gpu::Context>();
        _glWidget->makeCurrent();
        DependencyManager::get<DeferredLightingEffect>()->init();
        _glWidget->makeCurrent();
        _initContext.create();
        _glWidget->doneCurrent();
        auto displayPlugins = PluginManager::getInstance()->getDisplayPlugins();
        _displayPlugins.push_back(*displayPlugins.begin());
        for (auto displayPlugin : displayPlugins) {
            if (!displayPlugin->isSupported()) {
                continue;
            }

            if (displayPlugin->isHmd()) {
                _displayPlugins.push_back(displayPlugin);
                break;
            }
        }
        for (auto displayPlugin : _displayPlugins) {
            displayPlugin->setContainer(this);
            displayPlugin->setContext(_gpuContext);
            displayPlugin->init();
        }
        _displayPluginItr = _displayPlugins.begin();
        _activeDisplayPlugin = *_displayPluginItr;
        _glWidget->doneCurrent();
        if (!_activeDisplayPlugin->activate()) {
            qFatal("Could not activate display plugin");
        }

        _initContext.makeCurrent();
        QTimer* idleTimer = new QTimer(_glWidget);
        connect(idleTimer, &QTimer::timeout, [this] {
            idle();
        });
        idleTimer->start(0);

        connect(this, &QCoreApplication::aboutToQuit, [this, idleTimer] {
            _ready = false;
            idleTimer->stop();
            idleTimer->deleteLater();
            shutdown();
        });

    }

    virtual void shutdown() {
        _ready = false;
        _window->saveGeometry();
        _activeDisplayPlugin->deactivate();
        DependencyManager::destroy<DeferredLightingEffect>();
        DependencyManager::destroy<AnimationCache>();
        DependencyManager::destroy<FramebufferCache>();
        DependencyManager::destroy<TextureCache>();
        DependencyManager::destroy<ModelCache>();
        DependencyManager::destroy<GeometryCache>();
        DependencyManager::destroy<ScriptCache>();
        ResourceManager::cleanup();
        // remove the NodeList from the DependencyManager
        DependencyManager::destroy<NodeList>();
    }

    void idle() {
        if (!_ready) {
            return;
        }
        if (!_window->isVisible()) {
            return;
        }
        if (_renderCount != 0 && _renderCount >= _activeDisplayPlugin->presentCount()) {
            return;
        }
        _renderCount = _activeDisplayPlugin->presentCount();
        update();
        render();
    }

    virtual void update() {
        static auto presentRate = _activeDisplayPlugin->presentRate();
        if (_activeDisplayPlugin->presentRate() != presentRate) {
            presentRate = _activeDisplayPlugin->presentRate();
            QString title = QString("FPS %1 TextureMemory GPU %3 CPU %4")
                .arg(presentRate)
                .arg(toHumanSize(gpu::Context::getTextureGPUMemoryUsage(), 2))
                .arg(toHumanSize(gpu::Texture::getTextureCPUMemoryUsage(), 2));
            _window->setWindowTitle(title);
        }
    }
    virtual void render() = 0;


protected: 
    // PluginContainer interface
    ui::Menu* getPrimaryMenu() override { return nullptr; }
    void showDisplayPluginsTools(bool show) override {
    }
    void requestReset() override {
    }
    bool makeRenderingContextCurrent() override {
        return true;
    }
    GLWidget* getPrimaryWidget() override {
        return _glWidget;
    }
    MainWindow* getPrimaryWindow() override {
        return _window;
    }
    QOpenGLContext* getPrimaryContext() override {
        return _glWidget->context()->qglContext();
    }
    bool isForeground() const override {
        return true;
    }
    DisplayPluginPointer getActiveDisplayPlugin() const override {
        return _activeDisplayPlugin;
    }

protected:
    bool _ready { false };
    gpu::ContextPointer _gpuContext; // initialized during window creation
    DisplayPluginList _displayPlugins;
    DisplayPluginList::iterator _displayPluginItr;
    DisplayPluginPointer _activeDisplayPlugin;
    GLWidget* _glWidget { nullptr };
    MainWindow* _window { nullptr };
    gl::OffscreenContext _initContext;
    size_t _renderCount { 0 };
};


class RendererApp : public GLTestApp, public AbstractViewStateInterface {
    using Parent = GLTestApp;
public:
    RendererApp(int& argc, char** argv) : GLTestApp(argc, argv) {
        QThreadPool::globalInstance()->setMaxThreadCount(2);
        AbstractViewStateInterface::setInstance(this);
        _camera.movementSpeed = 50.0f;
        _octree = DependencyManager::set<EntityTreeRenderer>(false, this, nullptr);
        _octree->init();
        // Prevent web entities from rendering
        REGISTER_ENTITY_TYPE_WITH_FACTORY(Web, WebEntityItem::factory);
        PluginContainer* pluginContainer = dynamic_cast<PluginContainer*>(this); // set the container for any plugins that care
        PluginManager::getInstance()->setContainer(pluginContainer);

        DependencyManager::set<ParentFinder>(_octree->getTree());
        getEntities()->setViewFrustum(_viewFrustum);
        auto nodeList = DependencyManager::get<LimitedNodeList>();
        NodePermissions permissions;
        permissions.setAll(true);
        nodeList->setPermissions(permissions);

#if 0
        {
            SimpleEntitySimulationPointer simpleSimulation { new SimpleEntitySimulation() };
            simpleSimulation->setEntityTree(_octree->getTree());
            _octree->getTree()->setSimulation(simpleSimulation);
            _entitySimulation = simpleSimulation;
        }
#endif

        ResourceManager::init();

        // Render engine init
        _renderEngine->addJob<RenderShadowTask>("RenderShadowTask", _cullFunctor);
        const auto items = _renderEngine->addJob<RenderFetchCullSortTask>("FetchCullSort", _cullFunctor);
        assert(items.canCast<RenderFetchCullSortTask::Output>());
        static const QString RENDER_FORWARD = "HIFI_RENDER_FORWARD";
        if (QProcessEnvironment::systemEnvironment().contains(RENDER_FORWARD)) {
            _renderEngine->addJob<RenderForwardTask>("RenderForwardTask", items.get<RenderFetchCullSortTask::Output>());
        } else {
            _renderEngine->addJob<RenderDeferredTask>("RenderDeferredTask", items.get<RenderFetchCullSortTask::Output>());
        }
        _renderEngine->load();
        _renderEngine->registerScene(_main3DScene);

        // Render engine library init
        //reloadScene();
        //restorePosition();

        _ready = true;
    }

    void shutdown() override {
        getEntities()->shutdown(); // tell the entities system we're shutting down, so it will stop running scripts
        _renderEngine.reset();
        _main3DScene.reset();
        EntityTreePointer tree = getEntities()->getTree();
        tree->setSimulation(nullptr);
        Parent::shutdown();
    }

    void loadCommands(const QString& filename) {
        QFileInfo fileInfo(filename);
        if (!fileInfo.exists()) {
            return;
        }
#if 0
        _commandPath = fileInfo.absolutePath();
        _commands = FileUtils::readLines(filename);
        _commandIndex = 0;
#endif
    }

protected:

    bool event(QEvent* e) override {
        switch (e->type()) {
        case QEvent::KeyPress:
            keyPressEvent((QKeyEvent*)e);
            break;
        case QEvent::KeyRelease:
            keyReleaseEvent((QKeyEvent*)e);
            break;
        case QEvent::MouseMove:
            mouseMoveEvent((QMouseEvent*)e);
            break;
        }
        return Parent::event(e);
    }

    void keyPressEvent(QKeyEvent* event) {
        switch (event->key()) {
            case Qt::Key_F1:
                importScene();
                return;

            case Qt::Key_F2:
                reloadScene();
                return;

            case Qt::Key_F4:
                cycleMode();
                return;

            case Qt::Key_F5:
                goTo();
                return;

            case Qt::Key_F6:
                savePosition();
                return;

            case Qt::Key_F7:
                restorePosition();
                return;

            case Qt::Key_F8:
                resetPosition();
                return;

            case Qt::Key_F9:
                toggleCulling();
                return;

            case Qt::Key_Home:
                gpu::Texture::setAllowedGPUMemoryUsage(0);
                return;

            case Qt::Key_End:
                gpu::Texture::setAllowedGPUMemoryUsage(MB_TO_BYTES(64));
                return;

            default:
                break;
        }
        _camera.onKeyPress(event);
    }

    bool isAboutToQuit() const {
        return false;
    }

    void keyReleaseEvent(QKeyEvent* event) {
        _camera.onKeyRelease(event);
    }

    void mouseMoveEvent(QMouseEvent* event) {
        _camera.onMouseMove(event);
    }

    void parsePath(const QString& viewpointString) {
        static const QString FLOAT_REGEX_STRING = "([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)";
        static const QString SPACED_COMMA_REGEX_STRING = "\\s*,\\s*";
        static const QString POSITION_REGEX_STRING = QString("\\/") + FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING +
            FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING + FLOAT_REGEX_STRING + "\\s*(?:$|\\/)";
        static const QString QUAT_REGEX_STRING = QString("\\/") + FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING +
            FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING + FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING +
            FLOAT_REGEX_STRING + "\\s*$";

        static QRegExp orientationRegex(QUAT_REGEX_STRING);
        static QRegExp positionRegex(POSITION_REGEX_STRING);

        if (positionRegex.indexIn(viewpointString) != -1) {
            // we have at least a position, so emit our signal to say we need to change position
            glm::vec3 newPosition(positionRegex.cap(1).toFloat(),
                positionRegex.cap(2).toFloat(),
                positionRegex.cap(3).toFloat());
            _camera.setPosition(newPosition);

            if (!glm::any(glm::isnan(newPosition))) {
                // we may also have an orientation
                if (viewpointString[positionRegex.matchedLength() - 1] == QChar('/')
                    && orientationRegex.indexIn(viewpointString, positionRegex.matchedLength() - 1) != -1) {

                    glm::vec4 v = glm::vec4(
                        orientationRegex.cap(1).toFloat(),
                        orientationRegex.cap(2).toFloat(),
                        orientationRegex.cap(3).toFloat(),
                        orientationRegex.cap(4).toFloat());
                    if (!glm::any(glm::isnan(v))) {
                        _camera.setRotation(glm::normalize(glm::quat(v.w, v.x, v.y, v.z)));
                    }
                }
            }
        }
    }

    void importScene(const QString& fileName) {
        auto assetClient = DependencyManager::get<AssetClient>();
        QFileInfo fileInfo(fileName);
        QString atpPath = fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".atp";
        qDebug() << atpPath;
        QFileInfo atpPathInfo(atpPath);
        if (atpPathInfo.exists()) {
            QString atpUrl = QUrl::fromLocalFile(atpPath).toString();
            ResourceManager::setUrlPrefixOverride("atp:/", atpUrl + "/");
        }
        _settings.setValue(LAST_SCENE_KEY, fileName);
        _octree->clear();
        _octree->getTree()->readFromURL(fileName);
    }

    void importScene() {
        auto lastScene = _settings.value(LAST_SCENE_KEY);
        QString openDir;
        if (lastScene.isValid()) {
            QFileInfo lastSceneInfo(lastScene.toString());
            if (lastSceneInfo.absoluteDir().exists()) {
                openDir = lastSceneInfo.absolutePath();
            }
        }

        QString fileName = QFileDialog::getOpenFileName(nullptr, tr("Open File"), openDir, tr("Hifi Exports (*.json *.svo)"));
        if (fileName.isNull()) {
            return;
        }
        importScene(fileName);
    }

    void goTo() {
        QString destination = QInputDialog::getText(nullptr, tr("Go To Location"), "Enter path");
        if (destination.isNull()) {
            return;
        }
        parsePath(destination);
    }

    void reloadScene() {
        QVariant lastScene = _settings.value(LAST_SCENE_KEY);
        if (lastScene.isValid()) {
            importScene(lastScene.toString());
        }
    }

    void savePosition() {
        // /-17.2049,-8.08629,-19.4153/0,-0.48551,0,0.874231
        glm::quat q = _camera.getOrientation();
        glm::vec3 v = _camera.position;
        QString viewpoint = QString("/%1,%2,%3/%4,%5,%6,%7").
            arg(v.x).arg(v.y).arg(v.z).
            arg(q.x).arg(q.y).arg(q.z).arg(q.w);
        _settings.setValue(LAST_LOCATION_KEY, viewpoint);
    }

    void restorePosition() {
        // /-17.2049,-8.08629,-19.4153/0,-0.48551,0,0.874231
        QVariant viewpoint = _settings.value(LAST_LOCATION_KEY);
        if (viewpoint.isValid()) {
            parsePath(viewpoint.toString());
        }
    }

    void resetPosition() {
        _camera.setRotation(glm::quat());
        _camera.setPosition(vec3());
    }

    void toggleCulling() {
        _cullingEnabled = !_cullingEnabled;
    }

    void cycleMode() {
        if (_displayPlugins.end() == ++_displayPluginItr) {
            _displayPluginItr = _displayPlugins.begin();
        }

        auto newDisplayPlugin = *_displayPluginItr;
        if (newDisplayPlugin == _activeDisplayPlugin) {
            return;
        }

        // Default to the first item on the list, in case none of the menu items match

        if (_activeDisplayPlugin) {
            _activeDisplayPlugin->deactivate();
            _activeDisplayPlugin.reset();
        }

        _activeDisplayPlugin = newDisplayPlugin;
        bool active = _activeDisplayPlugin->activate();
        if (!active) {
            qFatal("Failed to activate display plugin");
        }
        //getApplicationCompositor().setDisplayPlugin(newDisplayPlugin);
    }

protected:
    // AbstractViewState interface
    void copyCurrentViewFrustum(ViewFrustum& viewOut) const override {
        viewOut = _viewFrustum;
    }
    void copyShadowViewFrustum(ViewFrustum& viewOut) const override {
        viewOut = _shadowViewFrustum;
    }
    QThread* getMainThread() override {
        return QThread::currentThread();
    }
    bool isHMDMode() const override {
        return _activeDisplayPlugin->isHmd();
    }
    PickRay computePickRay(float x, float y) const override {
        return PickRay();
    }
    glm::vec3 getAvatarPosition() const override {
        return vec3();
    }
    void postLambdaEvent(std::function<void()> f) override {}
    qreal getDevicePixelRatio() override {
        return 1.0f;
    }
    render::ScenePointer getMain3DScene() override {
        return _main3DScene;
    }
    render::EnginePointer getRenderEngine() override {
        return _renderEngine;
    }
    void pushPostUpdateLambda(void* key, std::function<void()> func) override {
        _postUpdateLambdas[key] = func;
    }

    QSharedPointer<EntityTreeRenderer> getEntities() {
        return _octree;
    }

    class EntityUpdateOperator : public RecurseOctreeOperator {
    public:
        EntityUpdateOperator(const qint64& now) : now(now) {}
        bool preRecursion(OctreeElementPointer element) override { return true; }
        bool postRecursion(OctreeElementPointer element) override {
            EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
            entityTreeElement->forEachEntity([&](EntityItemPointer entityItem) {
                if (!entityItem->isParentIDValid()) {
                    return;  // we weren't able to resolve a parent from _parentID, so don't save this entity.
                }
                entityItem->update(now);
            });
            return true;
        }
        const qint64& now;
    };

#if 0
    void runCommand(const QString& command) {
        qDebug() << "Running command: " << command;
        QStringList commandParams = command.split(QRegularExpression(QString("\\s")));
        QString verb = commandParams[0].toLower();
        if (verb == "loop") {
            if (commandParams.length() > 1) {
                int maxLoops = commandParams[1].toInt();
                if (maxLoops < ++_commandLoops) {
                    qDebug() << "Exceeded loop count";
                    return;
                }
            }
            _commandIndex = 0;
        } else if (verb == "wait") {
            if (commandParams.length() < 2) {
                qDebug() << "No wait time specified";
                return;
                }
            int seconds = commandParams[1].toInt();
            _nextCommandTime = usecTimestampNow() + seconds * USECS_PER_SECOND;
    } else if (verb == "load") {
        if (commandParams.length() < 2) {
            qDebug() << "No load file specified";
            return;
        }
        QString file = commandParams[1];
        if (QFileInfo(file).isRelative()) {
            file = _commandPath + "/" + file;
        }
        if (!QFileInfo(file).exists()) {
            qDebug() << "Cannot find scene file " + file;
            return;
        }

        importScene(file);
    } else if (verb == "go") {
        if (commandParams.length() < 2) {
            qDebug() << "No destination specified for go command";
            return;
        }
        parsePath(commandParams[1]);
    } else {
        qDebug() << "Unknown command " << command;
    }
}

    void runNextCommand(quint64 now) {
        if (_commands.empty()) {
            return;
        }

        if (_commandIndex >= _commands.size()) {
            _commands.clear();
            return;
        }

        if (now < _nextCommandTime) {
            return;
        }

        _nextCommandTime = 0;
        QString command = _commands[_commandIndex++];
        runCommand(command);
    }

    void update() {
        auto now = usecTimestampNow();
        static auto last = now;

        runNextCommand(now);
#endif
    void update() override {
        Parent::update();
        auto now = usecTimestampNow();
        static auto last = now;

        float delta = now - last;
        _camera.setAspectRatio(_activeDisplayPlugin->getRecommendedAspectRatio());

        auto pose = _activeDisplayPlugin->getHeadPose();
        // Update the camera
        _camera.update(delta / USECS_PER_SECOND);
        {
            _viewFrustum = ViewFrustum();
            _viewFrustum.setProjection(_camera.matrices.perspective);
            auto view = pose * glm::inverse(_camera.matrices.view);
            _viewFrustum.setPosition(glm::vec3(view[3]));
            _viewFrustum.setOrientation(glm::quat_cast(view));
            // Failing to do the calculation of the bound planes causes everything to be considered inside the frustum
            if (_cullingEnabled) {
                _viewFrustum.calculate();
            }
        }

        getEntities()->setViewFrustum(_viewFrustum);
        EntityUpdateOperator updateOperator(now);
        //getEntities()->getTree()->recurseTreeWithOperator(&updateOperator);
        {
            for (auto& iter : _postUpdateLambdas) {
                iter.second();
            }
            _postUpdateLambdas.clear();
        }

        last = now;

        getEntities()->update();

        // The pending changes collecting the changes here
        render::PendingChanges pendingChanges;

        // FIXME: Move this out of here!, Background / skybox should be driven by the enityt content just like the other entities
        // Background rendering decision
        if (!render::Item::isValidID(BackgroundRenderData::_item)) {
            auto backgroundRenderData = std::make_shared<BackgroundRenderData>();
            auto backgroundRenderPayload = std::make_shared<BackgroundRenderData::Payload>(backgroundRenderData);
            BackgroundRenderData::_item = _main3DScene->allocateID();
            pendingChanges.resetItem(BackgroundRenderData::_item, backgroundRenderPayload);
        }
        // Setup the current Zone Entity lighting
        {
            auto stage = DependencyManager::get<SceneScriptingInterface>()->getSkyStage();
            DependencyManager::get<DeferredLightingEffect>()->setGlobalLight(stage->getSunLight());
        }

        {
            PerformanceTimer perfTimer("SceneProcessPendingChanges");
            _main3DScene->enqueuePendingChanges(pendingChanges);

            _main3DScene->processPendingChangesQueue();
        }

    }

    void render() override {
        RenderArgs renderArgs(_gpuContext, _octree, DEFAULT_OCTREE_SIZE_SCALE,
            0, RenderArgs::DEFAULT_RENDER_MODE,
            RenderArgs::MONO, RenderArgs::RENDER_DEBUG_NONE);

        if (!_activeDisplayPlugin->isStereo()) {
            renderArgs._context->enableStereo(false);
        } else {
            renderArgs._context->enableStereo(true);
            mat4 eyeOffsets[2];
            mat4 eyeProjections[2];
            for_each_eye([&](Eye eye){
                eyeProjections[eye] = _activeDisplayPlugin->getEyeProjection(eye, _camera.matrices.perspective);
                eyeOffsets[eye] = _activeDisplayPlugin->getEyeToHeadTransform(eye);
            });
            renderArgs._context->setStereoProjections(eyeProjections);
            renderArgs._context->setStereoViews(eyeOffsets);
        }

        auto renderSize = _activeDisplayPlugin->getRecommendedRenderSize();
        auto framebufferCache = DependencyManager::get<FramebufferCache>();
        framebufferCache->setFrameBufferSize(fromGlm(renderSize));
        renderArgs._blitFramebuffer = framebufferCache->getFramebuffer();
        // Viewport is assigned to the size of the framebuffer
        renderArgs._viewport = ivec4(ivec2(0), renderSize);
        renderArgs.setViewFrustum(_viewFrustum);

#if 0
         Final framebuffer that will be handled to the display-plugin
        if (_fps != _renderThread._fps) {
            _fps = _renderThread._fps;
            updateText();
        }
#endif

        auto& gpuContext = renderArgs._context;
        gpuContext->beginFrame();
        gpu::doInBatch(gpuContext, [&](gpu::Batch& batch) {
            batch.resetStages();
        });
        PROFILE_RANGE(render, __FUNCTION__);
        PerformanceTimer perfTimer("draw");
        // The pending changes collecting the changes here
        render::PendingChanges pendingChanges;
        // Setup the current Zone Entity lighting
        DependencyManager::get<DeferredLightingEffect>()->setGlobalLight(_sunSkyStage.getSunLight());
        {
            PerformanceTimer perfTimer("SceneProcessPendingChanges");
            _main3DScene->enqueuePendingChanges(pendingChanges);
            _main3DScene->processPendingChangesQueue();
        }

        // For now every frame pass the renderContext
        {
            PerformanceTimer perfTimer("EngineRun");
            _renderEngine->getRenderContext()->args = &renderArgs;
            // Before the deferred pass, let's try to use the render engine
            _renderEngine->run();
        }
        auto frame = gpuContext->endFrame();
        frame->framebuffer = renderArgs._blitFramebuffer;
        frame->framebufferRecycler = [](const gpu::FramebufferPointer& framebuffer) {
            DependencyManager::get<FramebufferCache>()->releaseFramebuffer(framebuffer);
        };
        _activeDisplayPlugin->submitFrame(frame);
    }


#if 0
    void parsePath(const QString& viewpointString) {
        static const QString FLOAT_REGEX_STRING = "([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)";
        static const QString SPACED_COMMA_REGEX_STRING = "\\s*,\\s*";
        static const QString POSITION_REGEX_STRING = QString("\\/") + FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING +
            FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING + FLOAT_REGEX_STRING + "\\s*(?:$|\\/)";
        static const QString QUAT_REGEX_STRING = QString("\\/") + FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING +
            FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING + FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING +
            FLOAT_REGEX_STRING + "\\s*$";

        static QRegExp orientationRegex(QUAT_REGEX_STRING);
        static QRegExp positionRegex(POSITION_REGEX_STRING);

        if (positionRegex.indexIn(viewpointString) != -1) {
            // we have at least a position, so emit our signal to say we need to change position
            glm::vec3 newPosition(positionRegex.cap(1).toFloat(),
                positionRegex.cap(2).toFloat(),
                positionRegex.cap(3).toFloat());
            _camera.setPosition(newPosition);

            if (!glm::any(glm::isnan(newPosition))) {
                // we may also have an orientation
                if (viewpointString[positionRegex.matchedLength() - 1] == QChar('/')
                    && orientationRegex.indexIn(viewpointString, positionRegex.matchedLength() - 1) != -1) {

                    glm::vec4 v = glm::vec4(
                        orientationRegex.cap(1).toFloat(),
                        orientationRegex.cap(2).toFloat(),
                        orientationRegex.cap(3).toFloat(),
                        orientationRegex.cap(4).toFloat());
                    if (!glm::any(glm::isnan(v))) {
                        _camera.setRotation(glm::quat(v.w, v.x, v.y, v.z));
                    }
                }
            }
        }
    }

    void importScene(const QString& fileName) {
        auto assetClient = DependencyManager::get<AssetClient>();
        QFileInfo fileInfo(fileName);
        QString atpPath = fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".atp";
        qDebug() << atpPath;
        QFileInfo atpPathInfo(atpPath);
        if (atpPathInfo.exists()) {
            QString atpUrl = QUrl::fromLocalFile(atpPath).toString();
            ResourceManager::setUrlPrefixOverride("atp:/", atpUrl + "/");
        }
        _octree->clear();
        _octree->getTree()->readFromURL(fileName);
    }

    void importScene() {
        auto lastScene = _settings.value(LAST_SCENE_KEY);
        QString openDir;
        if (lastScene.isValid()) {
            QFileInfo lastSceneInfo(lastScene.toString());
            if (lastSceneInfo.absoluteDir().exists()) {
                openDir = lastSceneInfo.absolutePath();
            }
        }

        QString fileName = QFileDialog::getOpenFileName(nullptr, tr("Open File"), openDir, tr("Hifi Exports (*.json *.svo)"));
        if (fileName.isNull()) {
            return;
        }
        _settings.setValue(LAST_SCENE_KEY, fileName);
        importScene(fileName);
    }

    void goTo() {
        QString destination = QInputDialog::getText(nullptr, tr("Go To Location"), "Enter path");
        if (destination.isNull()) {
            return;
        }
        parsePath(destination);
    }

    void reloadScene() {
        QVariant lastScene = _settings.value(LAST_SCENE_KEY);
        if (lastScene.isValid()) {
            importScene(lastScene.toString());
        }
    }

    void savePosition() {
        // /-17.2049,-8.08629,-19.4153/0,-0.48551,0,0.874231
        glm::quat q = _camera.getOrientation();
        glm::vec3 v = _camera.position;
        QString viewpoint = QString("/%1,%2,%3/%4,%5,%6,%7").
            arg(v.x).arg(v.y).arg(v.z).
            arg(q.x).arg(q.y).arg(q.z).arg(q.w);
        _settings.setValue(LAST_LOCATION_KEY, viewpoint);
        _camera.setRotation(q);
    }

    void restorePosition() {
        // /-17.2049,-8.08629,-19.4153/0,-0.48551,0,0.874231
        QVariant viewpoint = _settings.value(LAST_LOCATION_KEY);
        if (viewpoint.isValid()) {
            parsePath(viewpoint.toString());
        }
    }

    void resetPosition() {
        _camera.setRotation(quat());
        _camera.setPosition(vec3());
    }

    void toggleCulling() {
        _cullingEnabled = !_cullingEnabled;
    }

    void cycleMode() {
        static auto defaultProjection = Camera().matrices.perspective;
        _renderMode = (RenderMode)((_renderMode + 1) % RENDER_MODE_COUNT);
        if (_renderMode == HMD) {
            _camera.matrices.perspective[0] = vec4 { 0.759056330, 0.000000000, 0.000000000, 0.000000000 };
            _camera.matrices.perspective[1] = vec4 { 0.000000000, 0.682773232, 0.000000000, 0.000000000 };
            _camera.matrices.perspective[2] = vec4 { -0.0580431037, -0.00619550655, -1.00000489, -1.00000000 };
            _camera.matrices.perspective[3] = vec4 { 0.000000000, 0.000000000, -0.0800003856, 0.000000000 };
        } else {
            _camera.matrices.perspective = defaultProjection;
            _camera.setAspectRatio((float)_size.width() / (float)_size.height());
        }
    }

    QSharedPointer<EntityTreeRenderer> getEntities() {
        return _octree;
    }

private:
    render::CullFunctor _cullFunctor { [&](const RenderArgs* args, const AABox& bounds)->bool{
#endif
protected:
    QSettings _settings;
    std::map<void*, std::function<void()>> _postUpdateLambdas;
    ViewFrustum _viewFrustum; // current state of view frustum, perspective, orientation, etc.
    ViewFrustum _shadowViewFrustum; // current state of view frustum, perspective, orientation, etc.
    render::EnginePointer _renderEngine { new render::Engine() };
    render::ScenePointer _main3DScene { new render::Scene(glm::vec3(-0.5f * (float)TREE_SCALE), (float)TREE_SCALE) };
    QSharedPointer<EntityTreeRenderer> _octree;
    static bool _cullingEnabled;
    QWindowCamera _camera;
    render::CullFunctor _cullFunctor { [&](const RenderArgs* args, const AABox& bounds)->bool {
        if (_cullingEnabled) {
            float renderAccuracy = calculateRenderAccuracy(args->getViewFrustum().getPosition(), bounds, args->_sizeScale, args->_boundaryLevelAdjust);
            return (renderAccuracy > 0.0f);
        } else {
            return true;
        }
    } };

    QSize _size;

    std::atomic<size_t> _renderCount{ 0 };
    gl::OffscreenContext _initContext;

    model::SunSkyStage _sunSkyStage;
    model::LightPointer _globalLight { std::make_shared<model::Light>() };
#if 0
    RenderThread _renderThread;
    bool _ready { false };
    EntitySimulationPointer _entitySimulation;

    QStringList _commands;
    QString _commandPath;
    int _commandLoops { 0 };
    int _commandIndex { -1 };
    uint64_t _nextCommandTime { 0 };

    //TextOverlay* _textOverlay;
    static bool _cullingEnabled;
    RenderMode _renderMode { NORMAL };
    QSharedPointer<EntityTreeRenderer> _octree;
#endif
};

bool RendererApp::_cullingEnabled { true };

const char * LOG_FILTER_RULES = R"V0G0N(
hifi.gpu=true
)V0G0N";

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QString logMessage = LogHandler::getInstance().printMessage((LogMsgType)type, context, message);

    if (!logMessage.isEmpty()) {
#ifdef Q_OS_WIN
        OutputDebugStringA(logMessage.toLocal8Bit().constData());
        OutputDebugStringA("\n");
#endif
        logger->addMessage(qPrintable(logMessage + "\n"));
    }
}

int main(int argc, char** argv) {
    RendererApp app(argc, argv);
    QLoggingCategory::setFilterRules(LOG_FILTER_RULES);
    logger.reset(new FileLogger());
    qInstallMessageHandler(messageHandler);
    //window.loadCommands("C:/Users/bdavis/Git/dreaming/exports2/commands.txt");
    app.exec();
    return 0;
}

#include "main.moc"

