//
//  ShaderTests.cpp
//  tests/octree/src
//
//  Created by Andrew Meadows on 2016.02.19
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "VulkanTests.h"

#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>

#include <QtCore/QJsonDocument>

#include <GLMHelpers.h>

#include <test-utils/GLMTestUtils.h>
#include <test-utils/QTestExtensions.h>
#include <shared/FileUtils.h>

#include <shaders/Shaders.h>
#include <gl/Config.h>
#include <gl/GLHelpers.h>
#include <vk/Helpers.h>
#include <gpu/vk/VKBackend.h>
#include <gl/QOpenGLContextWrapper.h>
#include <gl/Context.h>
#include <gl/GLWindow.h>


QTEST_MAIN(VulkanTests)

void VulkanTests::initTestCase() {
    //vks::Context::get().create();
    _glWindow = new GLWindow();
    getDefaultOpenGLSurfaceFormat();
    _glWindow->create();
    _glWindow->show();
    _glWindow->createContext();
    if (!_glWindow->makeCurrent()) {
        qFatal("Unable to make test GL context current");
    }
    ::gl::initModuleGl();

    //gpu::Context::init<gpu::gl::GLBackend>();
    //if (!_glContext->makeCurrent()) {
    //    qFatal("Unable to make test GL context current");
    //}
    //_gpuContext = std::make_shared<gpu::Context>();
}

void VulkanTests::cleanupTestCase() {
    qDebug() << "Done";
}


void VulkanTests::testVKBackend() {
    Q_ASSERT(vks::util::gl::contextSupported(nullptr));
    vks::util::gl::getUuids();
}
