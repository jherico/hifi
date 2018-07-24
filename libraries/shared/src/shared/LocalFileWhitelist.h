//
// Created by Bradley Austin Davis on 2018/07/24
// Copyright 2013-2018 High Fidelity, Inc.
//
// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_LocalFileWhitelist_h
#define hifi_LocalFileWhitelist_h

#include <QtCore/QString>
#include <QtCore/QList>

#include "ReadWriteLockable.h"
#include "../DependencyManager.h"

class QFileInfo;
class QDir;

class LocalFileWhitelist : public Dependency, protected ReadWriteLockable {
public:
    void addEntry(const QString& path);
    void addEntry(const QFileInfo& file);
    void addEntry(const QDir& dir);
    bool isWhitelisted(const QString& path) const;

private:
    QList<QString> _values;
};

#endif  //hifi_LocalFileWhitelist_h
