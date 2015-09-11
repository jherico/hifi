//
//  OverlayPanel.cpp
//  interface/src/ui/overlays
//
//  Created by Zander Otavka on 7/2/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OverlayPanel.h"

#include <QVariant>
#include <RegisteredMetaTypes.h>
#include <DependencyManager.h>
#include <EntityScriptingInterface.h>

#include "avatar/AvatarManager.h"
#include "avatar/MyAvatar.h"
#include "Application.h"
#include "Base3DOverlay.h"

PropertyBinding::PropertyBinding(QString avatar, QUuid entity) :
    avatar(avatar),
    entity(entity)
{
}

QJSValue propertyBindingToScriptValue(QJSEngine* engine, const PropertyBinding& value) {
    QJSValue obj = engine->newObject();

    if (value.avatar == "MyAvatar") {
        obj.setProperty("avatar", "MyAvatar");
    } else if (!value.entity.isNull()) {
        obj.setProperty("entity", value.entity.toString());
    }

    return obj;
}

void propertyBindingFromScriptValue(const QJSValue& object, PropertyBinding& value) {
    QJSValue avatar = object.property("avatar");
    QJSValue entity = object.property("entity");

    if (!avatar.isUndefined() && !avatar.isNull()) {
        value.avatar = avatar.toVariant().toString();
    } else if (!entity.isUndefined() && !entity.isNull()) {
        value.entity = entity.toVariant().toUuid();
    }
}


void OverlayPanel::addChild(unsigned int childId) {
    if (!_children.contains(childId)) {
        _children.append(childId);
    }
}

void OverlayPanel::removeChild(unsigned int childId) {
    if (_children.contains(childId)) {
        _children.removeOne(childId);
    }
}

QJSValue OverlayPanel::getProperty(const QString &property) {
    if (property == "anchorPosition") {
        return vec3toScriptValue(_scriptEngine, getAnchorPosition());
    }
    if (property == "anchorPositionBinding") {
        return propertyBindingToScriptValue(_scriptEngine,
                                            PropertyBinding(_anchorPositionBindMyAvatar ?
                                                            "MyAvatar" : "",
                                                            _anchorPositionBindEntity));
    }
    if (property == "anchorRotation") {
        return quatToScriptValue(_scriptEngine, getAnchorRotation());
    }
    if (property == "anchorRotationBinding") {
        return propertyBindingToScriptValue(_scriptEngine,
                                            PropertyBinding(_anchorRotationBindMyAvatar ?
                                                            "MyAvatar" : "",
                                                            _anchorRotationBindEntity));
    }
    if (property == "anchorScale") {
        return vec3toScriptValue(_scriptEngine, getAnchorScale());
    }
    if (property == "visible") {
        return getVisible();
    }
    if (property == "children") {
        QJSValue array = _scriptEngine->newArray(_children.length());
        for (int i = 0; i < _children.length(); i++) {
            array.setProperty(i, _children[i]);
        }
        return array;
    }

    QJSValue value = Billboardable::getProperty(_scriptEngine, property);
    if (!value.isUndefined()) {
        return value;
    }
    return PanelAttachable::getProperty(_scriptEngine, property);
}

void OverlayPanel::setProperties(const QJSValue &properties) {
    PanelAttachable::setProperties(properties);
    Billboardable::setProperties(properties);

    QJSValue anchorPosition = properties.property("anchorPosition");
    if (!anchorPosition.isUndefined() &&
        !anchorPosition.property("x").isUndefined() &&
        !anchorPosition.property("y").isUndefined() &&
        !anchorPosition.property("z").isUndefined()) {
        glm::vec3 newPosition;
        vec3FromScriptValue(anchorPosition, newPosition);
        setAnchorPosition(newPosition);
    }

    QJSValue anchorPositionBinding = properties.property("anchorPositionBinding");
    if (!anchorPositionBinding.isUndefined()) {
        PropertyBinding binding = {};
        propertyBindingFromScriptValue(anchorPositionBinding, binding);
        _anchorPositionBindMyAvatar = binding.avatar == "MyAvatar";
        _anchorPositionBindEntity = binding.entity;
    }

    QJSValue anchorRotation = properties.property("anchorRotation");
    if (!anchorRotation.isUndefined() &&
        !anchorRotation.property("x").isUndefined() &&
        !anchorRotation.property("y").isUndefined() &&
        !anchorRotation.property("z").isUndefined() &&
        !anchorRotation.property("w").isUndefined()) {
        glm::quat newRotation;
        quatFromScriptValue(anchorRotation, newRotation);
        setAnchorRotation(newRotation);
    }

    QJSValue anchorRotationBinding = properties.property("anchorRotationBinding");
    if (!anchorRotationBinding.isUndefined()) {
        PropertyBinding binding = {};
        propertyBindingFromScriptValue(anchorPositionBinding, binding);
        _anchorRotationBindMyAvatar = binding.avatar == "MyAvatar";
        _anchorRotationBindEntity = binding.entity;
    }

    QJSValue anchorScale = properties.property("anchorScale");
    if (!anchorScale.isUndefined()) {
        if (!anchorScale.property("x").isUndefined() &&
            !anchorScale.property("y").isUndefined() &&
            !anchorScale.property("z").isUndefined()) {
            glm::vec3 newScale;
            vec3FromScriptValue(anchorScale, newScale);
            setAnchorScale(newScale);
        } else {
            setAnchorScale(anchorScale.toVariant().toFloat());
        }
    }

    QJSValue visible = properties.property("visible");
    if (!visible.isUndefined()) {
        setVisible(visible.toBool());
    }
}

void OverlayPanel::applyTransformTo(Transform& transform, bool force) {
    if (force || usecTimestampNow() > _transformExpiry) {
        PanelAttachable::applyTransformTo(transform, true);
        if (!getParentPanel()) {
            if (_anchorPositionBindMyAvatar) {
                transform.setTranslation(DependencyManager::get<AvatarManager>()->getMyAvatar()
                                         ->getPosition());
            } else if (!_anchorPositionBindEntity.isNull()) {
                transform.setTranslation(DependencyManager::get<EntityScriptingInterface>()
                                         ->getEntityTree()->findEntityByID(_anchorPositionBindEntity)
                                         ->getPosition());
            } else {
                transform.setTranslation(getAnchorPosition());
            }

            if (_anchorRotationBindMyAvatar) {
                transform.setRotation(DependencyManager::get<AvatarManager>()->getMyAvatar()
                                      ->getOrientation());
            } else if (!_anchorRotationBindEntity.isNull()) {
                transform.setRotation(DependencyManager::get<EntityScriptingInterface>()
                                      ->getEntityTree()->findEntityByID(_anchorRotationBindEntity)
                                      ->getRotation());
            } else {
                transform.setRotation(getAnchorRotation());
            }

            transform.setScale(getAnchorScale());

            transform.postTranslate(getOffsetPosition());
            transform.postRotate(getOffsetRotation());
            transform.postScale(getOffsetScale());
        }
        pointTransformAtCamera(transform, getOffsetRotation());
    }
}
