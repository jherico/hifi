//
//  PanelAttachable.cpp
//  interface/src/ui/overlays
//
//  Created by Zander Otavka on 7/15/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PanelAttachable.h"

#include <RegisteredMetaTypes.h>

#include "OverlayPanel.h"

bool PanelAttachable::getParentVisible() const {
    if (getParentPanel()) {
        return getParentPanel()->getVisible() && getParentPanel()->getParentVisible();
    } else {
        return true;
    }
}

QJSValue PanelAttachable::getProperty(QJSEngine* scriptEngine, const QString &property) {
    if (property == "offsetPosition") {
        return vec3toScriptValue(scriptEngine, getOffsetPosition());
    }
    if (property == "offsetRotation") {
        return quatToScriptValue(scriptEngine, getOffsetRotation());
    }
    if (property == "offsetScale") {
        return vec3toScriptValue(scriptEngine, getOffsetScale());
    }
    return QJSValue();
}

void PanelAttachable::setProperties(const QJSValue &properties) {
    QJSValue offsetPosition = properties.property("offsetPosition");
    if (!offsetPosition.isUndefined() &&
        !offsetPosition.property("x").isUndefined() &&
        !offsetPosition.property("y").isUndefined() &&
        !offsetPosition.property("z").isUndefined()) {
        glm::vec3 newPosition;
        vec3FromScriptValue(offsetPosition, newPosition);
        setOffsetPosition(newPosition);
    }

    QJSValue offsetRotation = properties.property("offsetRotation");
    if (!offsetRotation.isUndefined() &&
        !offsetRotation.property("x").isUndefined() &&
        !offsetRotation.property("y").isUndefined() &&
        !offsetRotation.property("z").isUndefined() &&
        !offsetRotation.property("w").isUndefined()) {
        glm::quat newRotation;
        quatFromScriptValue(offsetRotation, newRotation);
        setOffsetRotation(newRotation);
    }

    QJSValue offsetScale = properties.property("offsetScale");
    if (!offsetScale.isUndefined()) {
        if (!offsetScale.property("x").isUndefined() &&
            !offsetScale.property("y").isUndefined() &&
            !offsetScale.property("z").isUndefined()) {
            glm::vec3 newScale;
            vec3FromScriptValue(offsetScale, newScale);
            setOffsetScale(newScale);
        } else {
            setOffsetScale(offsetScale.toVariant().toFloat());
        }
    }
}

void PanelAttachable::applyTransformTo(Transform& transform, bool force) {
    if (force || usecTimestampNow() > _transformExpiry) {
        const quint64 TRANSFORM_UPDATE_PERIOD = 100000; // frequency is 10 Hz
        _transformExpiry = usecTimestampNow() + TRANSFORM_UPDATE_PERIOD;
        if (getParentPanel()) {
            getParentPanel()->applyTransformTo(transform, true);
            transform.postTranslate(getOffsetPosition());
            transform.postRotate(getOffsetRotation());
            transform.postScale(getOffsetScale());
        }
    }
}
