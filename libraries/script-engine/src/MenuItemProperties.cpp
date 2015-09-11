//
//  MenuItemProperties.cpp
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>
#include <RegisteredMetaTypes.h>
#include "MenuItemProperties.h"

MenuItemProperties::MenuItemProperties() :
    menuName(""),
    menuItemName(""),
    shortcutKey(""),
    shortcutKeyEvent(),
    shortcutKeySequence(),
    position(UNSPECIFIED_POSITION),
    beforeItem(""),
    afterItem(""),
    isCheckable(false),
    isChecked(false),
    isSeparator(false) 
{
};

MenuItemProperties::MenuItemProperties(const QString& menuName, const QString& menuItemName,
                        const QString& shortcutKey, bool checkable, bool checked, bool separator) :
    menuName(menuName),
    menuItemName(menuItemName),
    shortcutKey(shortcutKey),
    shortcutKeyEvent(),
    shortcutKeySequence(shortcutKey),
    position(UNSPECIFIED_POSITION),
    beforeItem(""),
    afterItem(""),
    isCheckable(checkable),
    isChecked(checked),
    isSeparator(separator)
{
}

MenuItemProperties::MenuItemProperties(const QString& menuName, const QString& menuItemName, 
                        const KeyEvent& shortcutKeyEvent, bool checkable, bool checked, bool separator) :
    menuName(menuName),
    menuItemName(menuItemName),
    shortcutKey(""),
    shortcutKeyEvent(shortcutKeyEvent),
    shortcutKeySequence(shortcutKeyEvent),
    position(UNSPECIFIED_POSITION),
    beforeItem(""),
    afterItem(""),
    isCheckable(checkable),
    isChecked(checked),
    isSeparator(separator)
{
}

void registerMenuItemProperties(QJSEngine* engine) {
    // FIXME JSENGINE
#if 0
    qScriptRegisterMetaType(engine, menuItemPropertiesToScriptValue, menuItemPropertiesFromScriptValue);
#endif
}

QJSValue menuItemPropertiesToScriptValue(QJSEngine* engine, const MenuItemProperties& properties) {
    QJSValue obj = engine->newObject();
    // not supported
    return obj;
}

void menuItemPropertiesFromScriptValue(const QJSValue& object, MenuItemProperties& properties) {
    properties.menuName = object.property("menuName").toString();
    properties.menuItemName = object.property("menuItemName").toString();
    properties.isCheckable = object.property("isCheckable").toBool();
    properties.isChecked = object.property("isChecked").toBool();
    properties.isSeparator = object.property("isSeparator").toBool();
    
    // handle the shortcut key options in order...
    QJSValue shortcutKeyValue = object.property("shortcutKey");
    if (shortcutKeyValue.isString()) {
        properties.shortcutKey = shortcutKeyValue.toString();
        properties.shortcutKeySequence = properties.shortcutKey;
    } else {
        QJSValue shortcutKeyEventValue = object.property("shortcutKeyEvent");
        if (!shortcutKeyEventValue.isUndefined()) {
            KeyEvent::fromScriptValue(shortcutKeyEventValue, properties.shortcutKeyEvent);
            properties.shortcutKeySequence = properties.shortcutKeyEvent;
        }
    }

    if (!object.property("position").isUndefined()) {
        properties.position = object.property("position").toInt();
    }
    properties.beforeItem = object.property("beforeItem").toString();
    properties.afterItem = object.property("afterItem").toString();
}


