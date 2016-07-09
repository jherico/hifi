//
//  Created by Bradley Austin Davis on 2016/06/23
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Qml3DOverlay_h
#define hifi_Qml3DOverlay_h

#include "Billboard3DOverlay.h"

class OffscreenQmlSurface;

class Qml3DOverlay : public Billboard3DOverlay {
    Q_OBJECT

public:
    static QString const TYPE;
    virtual QString getType() const override { return TYPE; }

    Qml3DOverlay();
    Qml3DOverlay(const Qml3DOverlay* Qml3DOverlay);
    virtual ~Qml3DOverlay();

    virtual void render(RenderArgs* args) override;
    virtual const render::ShapeKey getShapeKey() override;

    virtual void update(float deltatime) override;

    // setters
    void setSource(const QString& url);

    void setProperties(const QVariantMap& properties) override;
    QVariant getProperty(const QString& property) override;

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, 
        BoxFace& face, glm::vec3& surfaceNormal) override;

    virtual Qml3DOverlay* createClone() const override;

private:
    OffscreenQmlSurface* _qmlSurface { nullptr };
    QMetaObject::Connection _connection;
    uint32_t _texture{ 0 };
    QString _source;
    float _dpi;
    vec2 _resolution{ 640, 480 };
};

#endif // hifi_Qml3DOverlay_h
