//
//  Created by Bradley Austin Davis on 2016/06/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Qml3DOverlay.h"

#include <QtGui/QOpenGLContext>
#include <QtQuick/QQuickItem>

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <GeometryUtil.h>
#include <TextureCache.h>
#include <PathUtils.h>
#include <gpu/Batch.h>
#include <RegisteredMetaTypes.h>
#include <AbstractViewStateInterface.h>

#include <gl/OffscreenQmlSurface.h>

static const float DPI = 30.47f;
static const float INCHES_TO_METERS = 1.0f / 39.3701f;

QString const Qml3DOverlay::TYPE = "qml3d";

Qml3DOverlay::Qml3DOverlay() : _dpi(DPI) { }

Qml3DOverlay::Qml3DOverlay(const Qml3DOverlay* Qml3DOverlay) :
    Billboard3DOverlay(Qml3DOverlay),
    _source(Qml3DOverlay->_source),
    _dpi(Qml3DOverlay->_dpi),
    _resolution(Qml3DOverlay->_resolution)
{
}

Qml3DOverlay::~Qml3DOverlay() {
    if (_qmlSurface) {
        _qmlSurface->pause();
        _qmlSurface->disconnect(_connection);
        // The lifetime of the QML surface MUST be managed by the main thread
        // Additionally, we MUST use local variables copied by value, rather than
        // member variables, since they would implicitly refer to a this that 
        // is no longer valid
        auto qmlSurface = _qmlSurface;
        AbstractViewStateInterface::instance()->postLambdaEvent([qmlSurface] {
            qmlSurface->deleteLater();
        });
    }
}

void Qml3DOverlay::update(float deltatime) {
    applyTransformTo(_transform);
}

void Qml3DOverlay::render(RenderArgs* args) {
    if (!_visible || !getParentVisible()) {
        return;
    }

    QOpenGLContext * currentContext = QOpenGLContext::currentContext();
    QSurface * currentSurface = currentContext->surface();
    if (!_qmlSurface) {
        _qmlSurface = new OffscreenQmlSurface();
        _qmlSurface->create(currentContext);
        _qmlSurface->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/"));
        _qmlSurface->load("WebEntity.qml");
        _qmlSurface->resume();
        _qmlSurface->getRootItem()->setProperty("source", _source);
        _qmlSurface->resize(QSize(_resolution.x, _resolution.y));
        _connection = QObject::connect(_qmlSurface, &OffscreenQmlSurface::textureUpdated, [&](GLuint textureId) {
            _texture = textureId;
        });
        currentContext->makeCurrent(currentSurface);
    }

    vec2 size = _resolution / _dpi * INCHES_TO_METERS;
    vec2 halfSize = size / 2.0f;
    vec4 color(toGlm(getColor()), getAlpha());

    applyTransformTo(_transform, true);
    Transform transform = _transform;
    if (glm::length2(getDimensions()) != 1.0f) {
        transform.postScale(vec3(getDimensions(), 1.0f));
    }

    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    if (_texture) {
        batch._glActiveBindTexture(GL_TEXTURE0, GL_TEXTURE_2D, _texture);
    } else {
        batch.setResourceTexture(0, DependencyManager::get<TextureCache>()->getWhiteTexture());
    }

    batch.setModelTransform(transform);
    DependencyManager::get<GeometryCache>()->renderQuad(batch, halfSize * -1.0f, halfSize, vec2(0), vec2(1), color);
    batch.setResourceTexture(0, args->_whiteTexture); // restore default white color after me
}

const render::ShapeKey Qml3DOverlay::getShapeKey() {
    auto builder = render::ShapeKey::Builder().withOwnPipeline();
    if (getAlpha() != 1.0f) {
        builder.withTranslucent();
    }
    return builder.build();
}

void Qml3DOverlay::setProperties(const QVariantMap& properties) {
    Billboard3DOverlay::setProperties(properties);

    auto urlValue = properties["source"];
    if (urlValue.isValid()) {
        QString newURL = urlValue.toString();
        if (newURL != _source) {
            setSource(newURL);
        }
    }

    auto resolution = properties["resolution"];
    if (resolution.isValid()) {
        bool valid;
        auto res = vec2FromVariant(resolution, valid);
        if (valid) {
            _resolution = res;
        }
    }


    auto dpi = properties["dpi"];
    if (dpi.isValid()) {
        _dpi = dpi.toFloat();
    }
}

QVariant Qml3DOverlay::getProperty(const QString& property) {
    if (property == "source") {
        return _source;
    }
    if (property == "dpi") {
        return _dpi;
    }
    return Billboard3DOverlay::getProperty(property);
}

void Qml3DOverlay::setSource(const QString& url) {
    _source = url;
    if (_qmlSurface) {
        _qmlSurface->executeOnUiThread([this, url]{
            _qmlSurface->getRootItem()->setProperty("source", url);
        });
    }

}

bool Qml3DOverlay::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, BoxFace& face, glm::vec3& surfaceNormal) {
    // FIXME - face and surfaceNormal not being returned

    // Make sure position and rotation is updated.
    applyTransformTo(_transform, true);
    vec2 size = _resolution / _dpi * INCHES_TO_METERS * vec2(getDimensions());
    // Produce the dimensions of the overlay based on the image's aspect ratio and the overlay's scale.
    return findRayRectangleIntersection(origin, direction, getRotation(), getPosition(), size, distance);
}

Qml3DOverlay* Qml3DOverlay::createClone() const {
    return new Qml3DOverlay(this);
}
