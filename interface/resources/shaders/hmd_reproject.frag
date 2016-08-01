//
//  Created by Bradley Austin Davis on 2016/07/11
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

uniform sampler2D sampler;

in vec2 varTexCoord0;

out vec4 FragColor;

void main() {
    FragColor = texture(sampler, varTexCoord0);
}