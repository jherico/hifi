//
//  Overlay2D.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Overlay2D.h"

#include <RegisteredMetaTypes.h>

Overlay2D::Overlay2D(const Overlay2D* overlay2D) :
    Overlay(overlay2D),
    _bounds(overlay2D->_bounds)
{
}

AABox Overlay2D::getBounds() const {
    return AABox(glm::vec3(_bounds.x(), _bounds.y(), 0.0f),
                 glm::vec3(_bounds.width(), _bounds.height(), 0.01f));
}

void Overlay2D::setProperties(const QJSValue& properties) {
    Overlay::setProperties(properties);
    
    QJSValue bounds = properties.property("bounds");
    if (!bounds.isUndefined()) {
        QRect boundsRect;
        boundsRect.setX(bounds.property("x").toVariant().toInt());
        boundsRect.setY(bounds.property("y").toVariant().toInt());
        boundsRect.setWidth(bounds.property("width").toVariant().toInt());
        boundsRect.setHeight(bounds.property("height").toVariant().toInt());
        setBounds(boundsRect);
    } else {
        QRect oldBounds = _bounds;
        QRect newBounds = oldBounds;
        
        if (!properties.property("x").isUndefined()) {
            newBounds.setX(properties.property("x").toVariant().toInt());
        } else {
            newBounds.setX(oldBounds.x());
        }
        if (!properties.property("y").isUndefined()) {
            newBounds.setY(properties.property("y").toVariant().toInt());
        } else {
            newBounds.setY(oldBounds.y());
        }
        if (!properties.property("width").isUndefined()) {
            newBounds.setWidth(properties.property("width").toVariant().toInt());
        } else {
            newBounds.setWidth(oldBounds.width());
        }
        if (!properties.property("height").isUndefined()) {
            newBounds.setHeight(properties.property("height").toVariant().toInt());
        } else {
            newBounds.setHeight(oldBounds.height());
        }
        setBounds(newBounds);
        //qDebug() << "set bounds to " << getBounds();
    }
}

QJSValue Overlay2D::getProperty(const QString& property) {
    if (property == "bounds") {
        return qRectToScriptValue(_scriptEngine, _bounds);
    }
    if (property == "x") {
        return _bounds.x();
    }
    if (property == "y") {
        return _bounds.y();
    }
    if (property == "width") {
        return _bounds.width();
    }
    if (property == "height") {
        return _bounds.height();
    }

    return Overlay::getProperty(property);
}
