//
//  VariantMapToScriptValue.h
//  libraries/shared/src/
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QVariant>

#include <QtQml/QJSEngine>

QJSValue variantToScriptValue(QVariant& qValue, QJSEngine& scriptEngine);
QJSValue variantMapToScriptValue(QVariantMap& variantMap, QJSEngine& scriptEngine);
QJSValue variantListToScriptValue(QVariantList& variantList, QJSEngine& scriptEngine);
