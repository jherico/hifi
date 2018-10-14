//
//  Pipeline.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Pipeline.h"
#include <math.h>
#include <QDebug>

using namespace gpu;

Pipeline::Pipeline(const ShaderPointer& program, const StatePointer& state, const FormatPointer& format) 
    : _program(program), _state(state), _format(format) 
{
}

Pipeline::~Pipeline()
{
}

Pipeline::Pointer Pipeline::create(const ShaderPointer& program, const StatePointer& state, const FormatPointer& format) {
    return Pointer(new Pipeline(program, state, format));
}
