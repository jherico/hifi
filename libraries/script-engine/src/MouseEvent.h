//
//  MouseEvent.h
//  script-engine/src
//
//  Created by Stephen Birarda on 2014-10-27.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MouseEvent_h
#define hifi_MouseEvent_h

#include <QMouseEvent>

class MouseEvent {
public:
    MouseEvent();
    MouseEvent(const QMouseEvent& event, const unsigned int deviceID = 0);
    
    static QJSValue toScriptValue(QJSEngine* engine, const MouseEvent& event);
    static void fromScriptValue(const QJSValue& object, MouseEvent& event);

    QJSValue toScriptValue(QJSEngine* engine) const { return MouseEvent::toScriptValue(engine, *this); }
    
    int x;
    int y;
    unsigned int deviceID;
    QString button;
    bool isLeftButton;
    bool isRightButton;
    bool isMiddleButton;
    bool isShifted;
    bool isControl;
    bool isMeta;
    bool isAlt;
};

Q_DECLARE_METATYPE(MouseEvent)

#endif // hifi_MouseEvent_h