//
// Created by Bradley Austin Davis on 2018/07/24
// Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LocalFileWhitelist.h"

#include <mutex>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include "../PathUtils.h"

void LocalFileWhitelist::addEntry(const QString& path) {
    if (path.isEmpty()) {
        return;
    }
    withWriteLock([&] { 
        _values.append(path); 
    });
}

void LocalFileWhitelist::addEntry(const QFileInfo& file) {
    if (!file.exists()) {
        return;
    }
    addEntry(file.canonicalFilePath());
}

void LocalFileWhitelist::addEntry(const QDir& dir) {
    addEntry(dir.canonicalPath());
}

bool LocalFileWhitelist::isWhitelisted(const QString& path) const {
    static std::once_flag once;
    std::call_once(once, [this] { 
        const_cast<LocalFileWhitelist&>(*this).addEntry(PathUtils::getApplicationPath());
#if defined(DEV_BUILD)
        // If we're in development mode, we may need to add the resources path as well
        if (!PathUtils::resourcesPath().startsWith(":")) {
            const_cast<LocalFileWhitelist&>(*this).addEntry(QDir(PathUtils::resourcesPath()));
        }
#endif
    });
    bool result = false;
    withReadLock([&] {
        for (const auto entry : _values) {
            if (path.startsWith(entry)) {
                result = true;
                break;
            }
        }
    });
    return result;
}
