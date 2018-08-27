//
//  Created by Bradley Austin Davis on 2018/09/17
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VulkanTests_h
#define hifi_VulkanTests_h

#include <QtTest/QtTest>
#include <gpu/Forward.h>

class GLWindow;

class VulkanTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testVKBackend();

private:
    GLWindow* _glWindow { nullptr };
    //gpu::ContextPointer _gpuContext;
};

#endif  // 
