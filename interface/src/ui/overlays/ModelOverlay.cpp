//
//  ModelOverlay.cpp
//
//
//  Created by Clement on 6/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelOverlay.h"
#include "EntityRig.h"

#include "Application.h"


QString const ModelOverlay::TYPE = "model";

ModelOverlay::ModelOverlay()
    : _model(std::make_shared<EntityRig>()),
      _modelTextures(QVariantMap()),
      _updateModel(false)
{
    _model.init();
    _isLoaded = false;
}

ModelOverlay::ModelOverlay(const ModelOverlay* modelOverlay) :
    Volume3DOverlay(modelOverlay),
    _model(std::make_shared<EntityRig>()),
    _modelTextures(QVariantMap()),
    _url(modelOverlay->_url),
    _updateModel(false)
{
    _model.init();
    if (_url.isValid()) {
        _updateModel = true;
        _isLoaded = false;
    }
}

void ModelOverlay::update(float deltatime) {
    if (_updateModel) {
        _updateModel = false;
        
        _model.setSnapModelToCenter(true);
        _model.setScale(getScale());
        _model.setRotation(getRotation());
        _model.setTranslation(getPosition());
        _model.setURL(_url);
        _model.simulate(deltatime, true);
    } else {
        _model.simulate(deltatime);
    }
    _isLoaded = _model.isActive();
}

bool ModelOverlay::addToScene(Overlay::Pointer overlay, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) {
    Volume3DOverlay::addToScene(overlay, scene, pendingChanges);
    _model.addToScene(scene, pendingChanges);
    return true;
}

void ModelOverlay::removeFromScene(Overlay::Pointer overlay, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) {
    Volume3DOverlay::removeFromScene(overlay, scene, pendingChanges);
    _model.removeFromScene(scene, pendingChanges);
}

void ModelOverlay::render(RenderArgs* args) {

    // check to see if when we added our model to the scene they were ready, if they were not ready, then
    // fix them up in the scene
    render::ScenePointer scene = Application::getInstance()->getMain3DScene();
    render::PendingChanges pendingChanges;
    if (_model.needsFixupInScene()) {
        _model.removeFromScene(scene, pendingChanges);
        _model.addToScene(scene, pendingChanges);
    }
    scene->enqueuePendingChanges(pendingChanges);

    if (!_visible) {
        return;
    }

    /*    
    if (_model.isActive()) {
        if (_model.isRenderable()) {
            float glowLevel = getGlowLevel();
            Glower* glower = NULL;
            if (glowLevel > 0.0f) {
                glower = new Glower(glowLevel);
            }
            _model.render(args, getAlpha());
            if (glower) {
                delete glower;
            }
        }
    }
    */
}

void ModelOverlay::setProperties(const QJSValue &properties) {
    auto position = getPosition();
    auto rotation = getRotation();
    auto scale = getDimensions();
    
    Volume3DOverlay::setProperties(properties);
    
    if (position != getPosition() || rotation != getRotation() || scale != getDimensions()) {
        _model.setScaleToFit(true, getScale());
        _updateModel = true;
    }
    
    QJSValue urlValue = properties.property("url");
    if (urlValue.isString()) {
        _url = urlValue.toString();
        _updateModel = true;
        _isLoaded = false;
    }
    
    QJSValue texturesValue = properties.property("textures");
    if (!texturesValue.isUndefined() && texturesValue.toVariant().canConvert(QVariant::Map)) {
        QVariantMap textureMap = texturesValue.toVariant().toMap();
        foreach(const QString& key, textureMap.keys()) {
            
            QUrl newTextureURL = textureMap[key].toUrl();
            qDebug() << "Updating texture named" << key << "to texture at URL" << newTextureURL;
            
            QMetaObject::invokeMethod(&_model, "setTextureWithNameToURL", Qt::AutoConnection,
                                      Q_ARG(const QString&, key),
                                      Q_ARG(const QUrl&, newTextureURL));

            _modelTextures[key] = newTextureURL;  // Keep local track of textures for getProperty()
        }
    }
}

QJSValue ModelOverlay::getProperty(const QString& property) {
    if (property == "url") {
        return _url.toString();
    }
    if (property == "dimensions") {
        return vec3toScriptValue(_scriptEngine, _model.getScaleToFitDimensions());
    }
    if (property == "textures") {
        if (_modelTextures.size() > 0) {
            QJSValue textures = _scriptEngine->newObject();
            foreach(const QString& key, _modelTextures.keys()) {
                textures.setProperty(key, _modelTextures[key].toString());
            }
            return textures;
        } else {
            return QJSValue();
        }
    }

    return Volume3DOverlay::getProperty(property);
}

bool ModelOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                        float& distance, BoxFace& face) {
    
    QString subMeshNameTemp;
    return _model.findRayIntersectionAgainstSubMeshes(origin, direction, distance, face, subMeshNameTemp);
}

bool ModelOverlay::findRayIntersectionExtraInfo(const glm::vec3& origin, const glm::vec3& direction,
                                                        float& distance, BoxFace& face, QString& extraInfo) {
    
    return _model.findRayIntersectionAgainstSubMeshes(origin, direction, distance, face, extraInfo);
}

ModelOverlay* ModelOverlay::createClone() const {
    return new ModelOverlay(this);
}
