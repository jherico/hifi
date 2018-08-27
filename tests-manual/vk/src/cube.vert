#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (std140, binding = 0) uniform bufferVals {
    mat4 mvp;
    mat4 projection;
    mat4 view;
} myBufferVals;

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in mat4 inTransform;

layout (location = 0) out vec4 outColor;

out gl_PerVertex { 
    vec4 gl_Position;
};


vec3 Ambient = vec3(0.5, 0.5, 0.5);
vec3 LightPosition = vec3(10.0, 4.0, 10.0);

void main() {
    mat4 modelView = myBufferVals.view * inTransform;
    //LightPosition = vec3(myBufferVals.view * vec4(LightPosition, 1.0))
    vec4 viewPosition = modelView * inPosition;
    vec3 viewNormal = normalize(vec4(modelView * vec4(inNormal.xyz, 0)).xyz);
    viewNormal.z = -viewNormal.z;
    vec3 pointToLight = normalize(LightPosition - viewPosition.xyz);

    outColor = clamp((inColor * clamp(dot(viewNormal, pointToLight), 0.0, 0.9)) + (inColor * vec4(vec3(0.1), 1.0)), vec4(0.0), vec4(1.0));
    gl_Position = myBufferVals.projection * viewPosition;

    // GL->VK conventions
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
