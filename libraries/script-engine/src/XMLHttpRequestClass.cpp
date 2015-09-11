//
//  XMLHttpRequestClass.cpp
//  libraries/script-engine/src/
//
//  Created by Ryan Huffman on 5/2/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  This class is an implementation of the XMLHttpRequest object for scripting use.  It provides a near-complete implementation
//  of the class described in the Mozilla docs: https://developer.mozilla.org/en-US/docs/Web/API/XMLHttpRequest
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QEventLoop>
#include <qurlquery.h>

#include <AccountManager.h>
#include <NetworkAccessManager.h>
#include <NetworkingConstants.h>

#include "ScriptEngine.h"
#include "XMLHttpRequestClass.h"

const QString METAVERSE_API_URL = NetworkingConstants::METAVERSE_SERVER_URL.toString() + "/api/";

Q_DECLARE_METATYPE(QByteArray*)

XMLHttpRequestClass::XMLHttpRequestClass(QJSEngine* engine) :
    _engine(engine),
    _async(true),
    _url(),
    _method(""),
    _responseType(""),
    _request(),
    _reply(NULL),
    _sendData(NULL),
    _rawResponseData(),
    _responseData(""),
    _onTimeout(QJSValue::NullValue),
    _onReadyStateChange(QJSValue::NullValue),
    _readyState(XMLHttpRequestClass::UNSENT),
    _errorCode(QNetworkReply::NoError),
    _timeout(0),
    _timer(this),
    _numRedirects(0) {

    _timer.setSingleShot(true);
}

XMLHttpRequestClass::~XMLHttpRequestClass() {
    if (_reply) { delete _reply; }
    if (_sendData) { delete _sendData; }
}
// FIXME JSENGINE
#if 0

QJSValue XMLHttpRequestClass::constructor(QScriptContext* context, QJSEngine* engine) {
    return engine->newQObject(new XMLHttpRequestClass(engine));
}

#endif
QJSValue XMLHttpRequestClass::getStatus() const {
    if (_reply) {
        return QJSValue(_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    } 
    return QJSValue(0);
}

QString XMLHttpRequestClass::getStatusText() const {
    if (_reply) {
        return _reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    }
    return "";
}

void XMLHttpRequestClass::abort() {
    abortRequest();
}

void XMLHttpRequestClass::setRequestHeader(const QString& name, const QString& value) {
    _request.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    _request.setRawHeader(QByteArray(name.toLatin1()), QByteArray(value.toLatin1()));
}

void XMLHttpRequestClass::requestMetaDataChanged() {
    QVariant redirect = _reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    // If this is a redirect, abort the current request and start a new one
    if (redirect.isValid() && _numRedirects < MAXIMUM_REDIRECTS) {
        _numRedirects++;
        abortRequest();

        QUrl newUrl = _url.resolved(redirect.toUrl().toString());
        _request.setUrl(newUrl);
        doSend();
    }
}

void XMLHttpRequestClass::requestDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (_readyState == OPENED && bytesReceived > 0) {
        setReadyState(HEADERS_RECEIVED);
        setReadyState(LOADING);
    }
}

QJSValue XMLHttpRequestClass::getAllResponseHeaders() const {
    if (_reply) {
        QList<QNetworkReply::RawHeaderPair> headerList = _reply->rawHeaderPairs();
        QByteArray headers;
        for (int i = 0; i < headerList.size(); i++) {
            headers.append(headerList[i].first);
            headers.append(": ");
            headers.append(headerList[i].second);
            headers.append("\n");
        }
        return QString(headers.data());
    }
    return QJSValue("");
}

QJSValue XMLHttpRequestClass::getResponseHeader(const QString& name) const {
    if (_reply && _reply->hasRawHeader(name.toLatin1())) {
        return QJSValue(QString(_reply->rawHeader(name.toLatin1())));
    }
    return QJSValue::NullValue;
}

void XMLHttpRequestClass::setReadyState(ReadyState readyState) {
    if (readyState != _readyState) {
        _readyState = readyState;
        if (_onReadyStateChange.isCallable()) {
            _onReadyStateChange.call();
        }
    }
}

void XMLHttpRequestClass::open(const QString& method, const QString& url, bool async, const QString& username,
                               const QString& password) {
    if (_readyState == UNSENT) {
        _method = method;
        _url.setUrl(url);
        _async = async;

        if (url.toLower().left(METAVERSE_API_URL.length()) == METAVERSE_API_URL) {
            AccountManager& accountManager = AccountManager::getInstance();
                
            if (accountManager.hasValidAccessToken()) {
                QUrlQuery urlQuery(_url.query());
                urlQuery.addQueryItem("access_token", accountManager.getAccountInfo().getAccessToken().token);
                _url.setQuery(urlQuery);
            }
                
        }
        if (!username.isEmpty()) {
            _url.setUserName(username);
        }
        if (!password.isEmpty()) {
            _url.setPassword(password);
        }
        _request.setUrl(_url);
        setReadyState(OPENED);
    }
}

void XMLHttpRequestClass::send() {
    send(QJSValue::NullValue);
}

void XMLHttpRequestClass::send(const QJSValue& data) {
    if (_readyState == OPENED && !_reply) {
        if (!data.isNull()) {
            _sendData = new QBuffer(this);
            if (data.isObject()) {
                // FIXME JSENGINE
#if 0
                QByteArray ba = qscriptvalue_cast<QByteArray>(data);
                _sendData->setData(ba);
#endif
            } else {
                _sendData->setData(data.toString().toUtf8());
            }
        }

        doSend();

        if (!_async) {
            QEventLoop loop;
            connect(this, SIGNAL(requestComplete()), &loop, SLOT(quit()));
            loop.exec();
        }
    }
}

void XMLHttpRequestClass::doSend() {
    
    _reply = NetworkAccessManager::getInstance().sendCustomRequest(_request, _method.toLatin1(), _sendData);
    connectToReply(_reply);

    if (_timeout > 0) {
        _timer.start(_timeout);
        connect(&_timer, SIGNAL(timeout()), this, SLOT(requestTimeout()));
    }
}

void XMLHttpRequestClass::requestTimeout() {
    if (_onTimeout.isCallable()) {
        _onTimeout.call();
    }
    abortRequest();
    _errorCode = QNetworkReply::TimeoutError;
    setReadyState(DONE);
    emit requestComplete();
}

void XMLHttpRequestClass::requestError(QNetworkReply::NetworkError code) {
}

void XMLHttpRequestClass::requestFinished() {
    disconnect(&_timer, SIGNAL(timeout()), this, SLOT(requestTimeout()));

    _errorCode = _reply->error();

    if (_errorCode == QNetworkReply::NoError) {
        _rawResponseData.append(_reply->readAll());

        if (_responseType == "json") {
            _responseData = _engine->evaluate("(" + QString(_rawResponseData.data()) + ")");
            if (_responseData.isError()) {
                // FIXME JSENGINE
#if 0
                _engine->clearExceptions();
#endif

                _responseData = QJSValue::NullValue;
            }
        } else if (_responseType == "arraybuffer") {
            // FIXME JSENGINE
#if 0
            QJSValue data = _engine->newVariant(QVariant::fromValue(_rawResponseData));
            _responseData = _engine->newObject(reinterpret_cast<ScriptEngine*>(_engine)->getArrayBufferClass(), data);
#endif
        } else {
            _responseData = QJSValue(QString(_rawResponseData.data()));
        }
    }

    setReadyState(DONE);
    emit requestComplete();
}

void XMLHttpRequestClass::abortRequest() {
    // Disconnect from signals we don't want to receive any longer.
    disconnect(&_timer, SIGNAL(timeout()), this, SLOT(requestTimeout()));
    if (_reply) {
        disconnectFromReply(_reply);
        _reply->abort();
        _reply->deleteLater();
        _reply = NULL;
    }
}

void XMLHttpRequestClass::connectToReply(QNetworkReply* reply) {
    connect(reply, SIGNAL(finished()), this, SLOT(requestFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(requestError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(requestDownloadProgress(qint64, qint64)));
    connect(reply, SIGNAL(metaDataChanged()), this, SLOT(requestMetaDataChanged()));
}

void XMLHttpRequestClass::disconnectFromReply(QNetworkReply* reply) {
    disconnect(reply, SIGNAL(finished()), this, SLOT(requestFinished()));
    disconnect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(requestError(QNetworkReply::NetworkError)));
    disconnect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(requestDownloadProgress(qint64, qint64)));
    disconnect(reply, SIGNAL(metaDataChanged()), this, SLOT(requestMetaDataChanged()));
}
