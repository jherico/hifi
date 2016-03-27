//
//  AssetRequest.cpp
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/24
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetRequest.h"

#include <algorithm>

#include <QtCore/QThread>

#include "AssetClient.h"
#include "NetworkLogging.h"
#include "NodeList.h"
#include "ResourceCache.h"

AssetRequest::AssetRequest(const QString& hash) :
    _hash(hash)
{
}

void AssetRequest::start() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "start", Qt::AutoConnection);
        return;
    }

    if (_state != NotStarted) {
        qCWarning(asset_client) << "AssetRequest already started.";
        return;
    }

    // in case we haven't parsed a valid hash, return an error now
    if (!isValidHash(_hash)) {
        _error = InvalidHash;
        _state = Finished;

        emit finished(this);
        return;
    }
    
    // Try to load from cache
    _data = loadFromCache(getUrl());
    if (!_data.isNull()) {
        _info.hash = _hash;
        _info.size = _data.size();
        _error = NoError;
        
        _state = Finished;
        emit finished(this);
        return;
    }
    
    _state = WaitingForInfo;
    
    QPointer<AssetRequest> that(this);
    auto assetClient = DependencyManager::get<AssetClient>();
    assetClient->getAssetInfo(_hash, [that](bool responseReceived, AssetServerError serverError, AssetInfo info) {
        if (that.isNull()) {
            return;
        }
        that->_info = info;

        if (!responseReceived) {
            that->_error = NetworkError;
        } else if (serverError != AssetServerError::NoError) {
            switch(serverError) {
                case AssetServerError::AssetNotFound:
                    that->_error = NotFound;
                    break;
                default:
                    that->_error = UnknownError;
                    break;
            }
        }

        if (that->_error != NoError) {
            qCWarning(asset_client) << "Got error retrieving asset info for" << that->_hash;
            that->_state = Finished;
            emit that->finished(that);
            
            return;
        }
        
        that->_state = WaitingForData;
        that->_data.resize(info.size);
        
        qCDebug(asset_client) << "Got size of " << that->_hash << " : " << info.size << " bytes";
        
        int start = 0, end = that->_info.size;
        
        auto assetClient = DependencyManager::get<AssetClient>();
        assetClient->getAsset(that->_hash, start, end, [that, start, end](bool responseReceived, AssetServerError serverError,
                                                                                const QByteArray& data) {
            if (that.isNull()) {
                return;
            }

            if (!responseReceived) {
                that->_error = NetworkError;
            } else if (serverError != AssetServerError::NoError) {
                switch (serverError) {
                    case AssetServerError::AssetNotFound:
                        that->_error = NotFound;
                        break;
                    case AssetServerError::InvalidByteRange:
                        that->_error = InvalidByteRange;
                        break;
                    default:
                        that->_error = UnknownError;
                        break;
                }
            } else {
                Q_ASSERT(data.size() == (end - start));
                
                // we need to check the hash of the received data to make sure it matches what we expect
                if (hashData(data).toHex() == that->_hash) {
                    memcpy(that->_data.data() + start, data.constData(), data.size());
                    that->_totalReceived += data.size();
                    emit that->progress(that->_totalReceived, that->_info.size);
                    
                    saveToCache(that->getUrl(), data);
                } else {
                    // hash doesn't match - we have an error
                    that->_error = HashVerificationFailed;
                }
                
            }
            
            if (that->_error != NoError) {
                qCWarning(asset_client) << "Got error retrieving asset" << that->_hash << "- error code" << that->_error;
            }
            
            that->_state = Finished;
            emit that->finished(that);
        }, [that](qint64 totalReceived, qint64 total) {
            if (!that.isNull()) {
                emit that->progress(totalReceived, total);
            }
        });
    });
}
