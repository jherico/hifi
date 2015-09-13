//
//  EntityItemID.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityItemID.h"

#include <QtCore/QObject>
#include <QDebug>
#include <BufferParser.h>
#include <udt/PacketHeaders.h>
#include <UUID.h>

#include <RegisteredMetaTypes.h>


EntityItemID::EntityItemID() : QUuid()
{
}


EntityItemID::EntityItemID(const QUuid& id) : QUuid(id)
{
}

// EntityItemID::EntityItemID(const EntityItemID& other) : QUuid(other)
// {
// }

EntityItemID EntityItemID::readEntityItemIDFromBuffer(const unsigned char* data, int bytesLeftToRead) {
    EntityItemID result;
    if (bytesLeftToRead >= NUM_BYTES_RFC4122_UUID) {
        BufferParser(data, bytesLeftToRead).readUuid(result);
    }
    return result;
}

QJSValue EntityItemID::toScriptValue(QJSEngine* engine) const { 
    return EntityItemIDtoScriptValue(engine, *this); 
}

QJSValue EntityItemIDtoScriptValue(QJSEngine* engine, const EntityItemID& id) {
    return quuidToScriptValue(engine, id);
}

void EntityItemIDfromScriptValue(const QJSValue &object, EntityItemID& id) {
    quuidFromScriptValue(object, id);
}
