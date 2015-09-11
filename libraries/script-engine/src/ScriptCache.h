//
//  ScriptCache.h
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 2015-03-30
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptCache_h
#define hifi_ScriptCache_h

#include <ResourceCache.h>
#include <shared/ReadWriteLockable.h>
class ScriptUser;
class Condition;

/// Interface for loading scripts
class ScriptCache : public QObject, public Dependency, public ReadWriteLockable {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    QString getScript(const QUrl& url, ScriptUser* scriptUser, bool& isPending, bool redownload = false);
    QString getScriptSynchronously(const QUrl& url, bool redownload = false);
    void deleteScript(const QUrl& url);
    void addScriptToBadScriptList(const QUrl& url) { _badScripts.insert(url); }
    bool isInBadScriptList(const QUrl& url) { return _badScripts.contains(url); }
    
private slots:
    void scriptDownloaded();
    
private:
    bool fetchCachedScript(const QUrl& url, QString& result);
    void fetchScript(const QUrl& url, bool reload, bool synchronous);
        
    ScriptCache(QObject* parent = NULL);
    
    QHash<QUrl, QString> _scriptCache;
    QMultiMap<QUrl, ScriptUser*> _scriptUsers;
    QSet<QUrl> _badScripts;
};

#endif // hifi_ScriptCache_h