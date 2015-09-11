//
//  ScriptCache.cpp
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 2015-03-30
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <assert.h>

#include <QtCore/QObject>
#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>
#include <QtCore/QThread>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkConfiguration>
#include <QtNetwork/QNetworkReply>

#include <SharedUtil.h>
#include <ThreadHelpers.h>
#include <shared/ReadWriteLockable.h>

#include "ScriptCache.h"
#include "ScriptEngine.h"
#include "ScriptEngineLogging.h"

QHash<QUrl, Condition*> _conditions;

ScriptCache::ScriptCache(QObject* parent) : QObject(parent) {
}

bool ScriptCache::fetchCachedScript(const QUrl& url, QString& scriptContents) {
    bool result = false;
    withReadLock([&] {
        if (_scriptCache.contains(url)) {
            qCDebug(scriptengine) << "Found script in cache:" << url.toString();
            scriptContents = _scriptCache[url];
            result = true;
        }
    });
    return result;
}

void ScriptCache::fetchScript(const QUrl& url, bool reload, bool synchronous) {
    Condition* condition{ nullptr };
    withWriteLock([&] {
        if (!_conditions.contains(url)) {
            _conditions.insert(url, new Condition());
            QCoreApplication::postEvent(qApp, new LambdaEvent([=] {
                auto request = ResourceManager::createResourceRequest(this, url);
                request->setCacheEnabled(!reload);
                connect(request, &ResourceRequest::finished, this, &ScriptCache::scriptDownloaded);
                request->send();
            }));
        } else {
            qCDebug(scriptengine) << "Already downloading script at:" << url.toString();
        }
        condition = _conditions[url];
        condition->increment();
    });
    if (synchronous) {
        Q_ASSERT(QThread::currentThread() != qApp->thread());
        condition->decrementAndWait();
    } else {
        condition->decrement();
    }
}

QString ScriptCache::getScript(const QUrl& url, ScriptUser* scriptUser, bool& isPending, bool reload) {
    Q_ASSERT(qApp->thread() == QThread::currentThread());
    QString scriptContents;
    if (!reload) {
        isPending = true;
        if (fetchCachedScript(url, scriptContents)) {
            isPending = false;
            _scriptUsers.insert(url, scriptUser);
            scriptUser->scriptContentsAvailable(url, scriptContents);
            return scriptContents;
        }
    }

    fetchScript(url, reload, false);
    return scriptContents;
}


QString ScriptCache::getScriptSynchronously(const QUrl& url, bool reload) {
    Q_ASSERT(qApp->thread() != QThread::currentThread());
    QString scriptContents;
    if (!reload && fetchCachedScript(url, scriptContents)) {
        return scriptContents;
    }

    fetchScript(url, reload, true);
    fetchCachedScript(url, scriptContents);
    return scriptContents;
}


void ScriptCache::deleteScript(const QUrl& url) {
    withWriteLock([&] {
        if (_scriptCache.contains(url)) {
            qCDebug(scriptengine) << "Delete script from cache:" << url.toString();
            _scriptCache.remove(url);
        }
    });
}

void ScriptCache::scriptDownloaded() {
    Condition* condition{ nullptr };
    withWriteLock([&] {
        ResourceRequest* req = qobject_cast<ResourceRequest*>(sender());
        QUrl url = req->getUrl();
        Q_ASSERT(_conditions.contains(url));
        condition = _conditions[url];
        _conditions.remove(url);

        QList<ScriptUser*> scriptUsers = _scriptUsers.values(url);
        _scriptUsers.remove(url);

        if (req->getResult() == ResourceRequest::Success) {
            _scriptCache[url] = req->getData();
            qCDebug(scriptengine) << "Done downloading script at:" << url.toString();
            foreach(ScriptUser* user, scriptUsers) {
                user->scriptContentsAvailable(url, _scriptCache[url]);
            }
        } else {
            qCWarning(scriptengine) << "Error loading script from URL " << url;
            foreach(ScriptUser* user, scriptUsers) {
                user->errorInLoadingScript(url);
            }
        }
        req->deleteLater();
    });
    // MEMORY LEAK
    condition->satisfy();
}


