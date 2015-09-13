//
//  Billboardable.h
//  interface/src/ui/overlays
//
//  Created by Zander Otavka on 8/7/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Billboardable_h
#define hifi_Billboardable_h


#include <QtQml/QJSEngine>

#include <Transform.h>

class Billboardable {
public:
    bool isFacingAvatar() const { return _isFacingAvatar; }
    void setIsFacingAvatar(bool isFacingAvatar) { _isFacingAvatar = isFacingAvatar; }

protected:
    void setProperties(const QJSValue& properties);
    QJSValue getProperty(QJSEngine* scriptEngine, const QString& property);

    void pointTransformAtCamera(Transform& transform, glm::quat offsetRotation = {1, 0, 0, 0});

private:
    bool _isFacingAvatar = false;
};

#endif // hifi_Billboardable_h
