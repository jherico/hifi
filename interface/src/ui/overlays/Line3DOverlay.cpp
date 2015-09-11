//
//  Line3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Line3DOverlay.h"

#include <GeometryCache.h>
#include <RegisteredMetaTypes.h>


QString const Line3DOverlay::TYPE = "line3d";

Line3DOverlay::Line3DOverlay() :
    _geometryCacheID(DependencyManager::get<GeometryCache>()->allocateID())
{
}

Line3DOverlay::Line3DOverlay(const Line3DOverlay* line3DOverlay) :
    Base3DOverlay(line3DOverlay),
    _end(line3DOverlay->_end),
    _geometryCacheID(DependencyManager::get<GeometryCache>()->allocateID())
{
}

Line3DOverlay::~Line3DOverlay() {
}

AABox Line3DOverlay::getBounds() const {
    auto extents = Extents{};
    extents.addPoint(_start);
    extents.addPoint(_end);
    extents.transform(_transform);
    
    return AABox(extents);
}

void Line3DOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glm::vec4 colorv4(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    auto batch = args->_batch;
    if (batch) {
        batch->setModelTransform(_transform);

        if (getIsDashedLine()) {
            // TODO: add support for color to renderDashedLine()
            DependencyManager::get<GeometryCache>()->renderDashedLine(*batch, _start, _end, colorv4, _geometryCacheID);
        } else {
            DependencyManager::get<GeometryCache>()->renderLine(*batch, _start, _end, colorv4, _geometryCacheID);
        }
    }
}

void Line3DOverlay::setProperties(const QJSValue& properties) {
    Base3DOverlay::setProperties(properties);

    QJSValue start = properties.property("start");
    // if "start" property was not there, check to see if they included aliases: startPoint
    if (start.isUndefined()) {
        start = properties.property("startPoint");
    } 
    if (!start.isUndefined()) {
        QJSValue x = start.property("x");
        QJSValue y = start.property("y");
        QJSValue z = start.property("z");
        if (!x.isUndefined() && !y.isUndefined() && !z.isUndefined()) {
            glm::vec3 newStart;
            vec3FromScriptValue(start, newStart);
            setStart(newStart);
        }
    }

    QJSValue end = properties.property("end");
    // if "end" property was not there, check to see if they included aliases: endPoint
    if (end.isUndefined()) {
        end = properties.property("endPoint");
    }
    if (!end.isUndefined()) {
        QJSValue x = end.property("x");
        QJSValue y = end.property("y");
        QJSValue z = end.property("z");
        if (!x.isUndefined() && !y.isUndefined() && !z.isUndefined()) {
            glm::vec3 newEnd;
            vec3FromScriptValue(start, newEnd);
            setEnd(newEnd);
        }
    }
}

QJSValue Line3DOverlay::getProperty(const QString& property) {
    if (property == "end" || property == "endPoint" || property == "p2") {
        return vec3toScriptValue(_scriptEngine, _end);
    }

    return Base3DOverlay::getProperty(property);
}

Line3DOverlay* Line3DOverlay::createClone() const {
    return new Line3DOverlay(this);
}
