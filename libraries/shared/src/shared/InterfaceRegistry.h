//
//  Created by Bradley Austin Davis on 2018/02/03
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_InterfaceRegistry_h
#define hifi_InterfaceRegistry_h


#include <string>
#include <unordered_map>
#include <mutex>

#include "../DependencyManager.h"

class QObject;

class InterfaceRegistry : public Dependency {

public:
    using ProviderLambda = std::function<QObject*(QObject*)>;
    using SimpleProviderLambda = std::function<QObject*()>;
    using ApplyLambda = std::function<void(const std::string&, QObject*)>;

    void registerInterface(const std::string& name, const ProviderLambda& provider);
    void registerInterface(const std::string& name, const SimpleProviderLambda& provider);
    void applyRegistry(QObject* parent, const ApplyLambda& callback) const;

private:
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    using Map = std::unordered_map<std::string, ProviderLambda>;

    Map _interfaces;
    mutable std::atomic<bool> _locked{ false };
    mutable Mutex _mutex;
};



#endif // hifi_InterfaceRegistry_h
