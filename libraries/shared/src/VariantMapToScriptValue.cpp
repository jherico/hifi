//
//  VariantMapToScriptValue.cpp
//  libraries/shared/src/
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>
#include "SharedLogging.h"
#include "VariantMapToScriptValue.h"


QJSValue variantToScriptValue(QVariant& qValue, QJSEngine& scriptEngine) {
    switch(qValue.type()) {
        case QVariant::Bool:
            return qValue.toBool();
            break;
        case QVariant::Int:
            return qValue.toInt();
            break;
        case QVariant::Double:
            return qValue.toDouble();
            break;
        case QVariant::String: {
            return qValue.toString();
            break;
        }
        case QVariant::Map: {
            QVariantMap childMap = qValue.toMap();
            return variantMapToScriptValue(childMap, scriptEngine);
            break;
        }
        case QVariant::List: {
            QVariantList childList = qValue.toList();
            return variantListToScriptValue(childList, scriptEngine);
            break;
        }
        default:
            qCDebug(shared) << "unhandled QScript type" << qValue.type();
            break;
    }

    return QJSValue();
}


QJSValue variantMapToScriptValue(QVariantMap& variantMap, QJSEngine& scriptEngine) {
    QJSValue scriptValue = scriptEngine.newObject();

    for (QVariantMap::const_iterator iter = variantMap.begin(); iter != variantMap.end(); ++iter) {
        QString key = iter.key();
        QVariant qValue = iter.value();
        scriptValue.setProperty(key, variantToScriptValue(qValue, scriptEngine));
    }

    return scriptValue;
}


QJSValue variantListToScriptValue(QVariantList& variantList, QJSEngine& scriptEngine) {
    QJSValue scriptValue = scriptEngine.newObject();

    scriptValue.setProperty("length", variantList.size());
    int i = 0;
    foreach (QVariant v, variantList) {
        scriptValue.setProperty(i++, variantToScriptValue(v, scriptEngine));
    }

    return scriptValue;
}
