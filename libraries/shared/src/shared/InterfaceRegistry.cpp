//
//  Created by Bradley Austin Davis on 2018/02/03
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterfaceRegistry.h"
#include "../SharedLogging.h"


void InterfaceRegistry::registerInterface(const std::string& name, const ProviderLambda& provider) {
    Lock lock(_mutex);

    // Once the registry has been used to apply the interface it can't be modified anymore
    if (_locked.load()) {
        qFatal("Registry has already been locked");
    }

    // Duplicates in the registry are disallowed
    if (_interfaces.count(name)) {
        qFatal("Registry already contains an interface with the name %s", name.c_str());
    }

    _interfaces.insert({ name, provider });
}

void InterfaceRegistry::registerInterface(const std::string& name, const SimpleProviderLambda& simpleProvider) {
    registerInterface(name, [=](QObject* parent)->QObject* {
        return simpleProvider();
    });
}

void InterfaceRegistry::applyRegistry(QObject* parent, const ApplyLambda& callback) const {
    Lock lock(_mutex);
    bool unlockedState = false;
    if (_locked.compare_exchange_strong(unlockedState, true)) {
        qCDebug(shared) << "Locking the interface registry with" << _interfaces.size() << "items";
    }
    for (const auto& entry : _interfaces) {
        const auto& name = entry.first;
        const auto& provider = entry.second;
        callback(name, provider(parent));
    }
}
