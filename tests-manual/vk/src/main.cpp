//
//  main.cpp
//  tests/gpu-test/src
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtGui/QGuiApplication>

#include "VKCubeWindow.h"

int main(int argc, char** argv) {    
    QGuiApplication app(argc, argv);

    CubeWindow* window = new CubeWindow();
    window->show();
    app.exec();
    return 0;
}

