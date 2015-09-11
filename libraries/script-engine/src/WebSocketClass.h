//
//  WebSocketClass.h
//  libraries/script-engine/src/
//
//  Created by Thijs Wenker on 8/4/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WebSocketClass_h
#define hifi_WebSocketClass_h

#include <QObject>
#include <QtQml/QJSEngine>
#include <QWebSocket>

class WebSocketClass : public QObject {
    Q_OBJECT
        Q_PROPERTY(QString binaryType READ getBinaryType WRITE setBinaryType)
        Q_PROPERTY(ulong bufferedAmount READ getBufferedAmount)
        Q_PROPERTY(QString extensions READ getExtensions)

        Q_PROPERTY(QJSValue onclose READ getOnClose WRITE setOnClose)
        Q_PROPERTY(QJSValue onerror READ getOnError WRITE setOnError)
        Q_PROPERTY(QJSValue onmessage READ getOnMessage WRITE setOnMessage)
        Q_PROPERTY(QJSValue onopen READ getOnOpen WRITE setOnOpen)

        Q_PROPERTY(QString protocol READ getProtocol)
        Q_PROPERTY(WebSocketClass::ReadyState readyState READ getReadyState)
        Q_PROPERTY(QString url READ getURL)

        Q_PROPERTY(WebSocketClass::ReadyState CONNECTING READ getConnecting CONSTANT)
        Q_PROPERTY(WebSocketClass::ReadyState OPEN READ getOpen CONSTANT)
        Q_PROPERTY(WebSocketClass::ReadyState CLOSING READ getClosing CONSTANT)
        Q_PROPERTY(WebSocketClass::ReadyState CLOSED READ getClosed CONSTANT)

public:
    WebSocketClass(QJSEngine* engine, QString url);
    WebSocketClass(QJSEngine* engine, QWebSocket* qWebSocket);
    ~WebSocketClass();

    // FIXME JSENGINE
#if 0
    static QJSValue constructor(QJSEngine* engine);
#endif

    enum ReadyState {
        CONNECTING = 0,
        OPEN,
        CLOSING,
        CLOSED
    };

    QWebSocket* getWebSocket() { return _webSocket; }

    ReadyState getConnecting() const { return CONNECTING; };
    ReadyState getOpen() const { return OPEN; };
    ReadyState getClosing() const { return CLOSING; };
    ReadyState getClosed() const { return CLOSED; };

    void setBinaryType(QString binaryType) { _binaryType = binaryType; }
    QString getBinaryType() { return _binaryType; }

    // extensions is a empty string until supported in QT WebSockets
    QString getExtensions() { return QString(); }

    // protocol is a empty string until supported in QT WebSockets
    QString getProtocol() { return QString(); }

    ulong getBufferedAmount() { return 0; }

    QString getURL() { return _webSocket->requestUrl().toDisplayString(); }

    ReadyState getReadyState() {
        switch (_webSocket->state()) {
            case QAbstractSocket::SocketState::HostLookupState:
            case QAbstractSocket::SocketState::ConnectingState:
                return CONNECTING;
            case QAbstractSocket::SocketState::ConnectedState:
            case QAbstractSocket::SocketState::BoundState:
            case QAbstractSocket::SocketState::ListeningState:
                return OPEN;
            case QAbstractSocket::SocketState::ClosingState:
                return CLOSING;
            case QAbstractSocket::SocketState::UnconnectedState:
            default:
                return CLOSED;
        }
    }

    void setOnClose(QJSValue eventFunction) { _onCloseEvent = eventFunction; }
    QJSValue getOnClose() { return _onCloseEvent; }

    void setOnError(QJSValue eventFunction) { _onErrorEvent = eventFunction; }
    QJSValue getOnError() { return _onErrorEvent; }

    void setOnMessage(QJSValue eventFunction) { _onMessageEvent = eventFunction; }
    QJSValue getOnMessage() { return _onMessageEvent; }

    void setOnOpen(QJSValue eventFunction) { _onOpenEvent = eventFunction; }
    QJSValue getOnOpen() { return _onOpenEvent; }

public slots:
    void send(QJSValue message);

    void close();
    void close(QWebSocketProtocol::CloseCode closeCode);
    void close(QWebSocketProtocol::CloseCode closeCode, QString reason);

private:
    QWebSocket* _webSocket;
    QJSEngine* _engine;

    QJSValue _onCloseEvent;
    QJSValue _onErrorEvent;
    QJSValue _onMessageEvent;
    QJSValue _onOpenEvent;

    QString _binaryType;

    void initialize();

private slots:
    void handleOnClose();
    void handleOnError(QAbstractSocket::SocketError error);
    void handleOnMessage(const QString& message);
    void handleOnOpen();

};

Q_DECLARE_METATYPE(QWebSocketProtocol::CloseCode);
Q_DECLARE_METATYPE(WebSocketClass::ReadyState);

QJSValue qWSCloseCodeToScriptValue(QJSEngine* engine, const QWebSocketProtocol::CloseCode& closeCode);
void qWSCloseCodeFromScriptValue(const QJSValue& object, QWebSocketProtocol::CloseCode& closeCode);

QJSValue webSocketToScriptValue(QJSEngine* engine, WebSocketClass* const &in);
void webSocketFromScriptValue(const QJSValue &object, WebSocketClass* &out);

QJSValue wscReadyStateToScriptValue(QJSEngine* engine, const WebSocketClass::ReadyState& readyState);
void wscReadyStateFromScriptValue(const QJSValue& object, WebSocketClass::ReadyState& readyState);

#endif // hifi_WebSocketClass_h
