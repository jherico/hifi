#version 450 core
#define GPU_GL450
#define GPU_SSBO_TRANSFORM_OBJECT
#define BITFIELD int
#define LAYOUT(X) layout(X)
#define LAYOUT_STD140(X) layout(std140, X)
#define LAYOUT_SET(S, X) layout(S, X)
#define LAYOUT_SET_STD140(S, X) layout(std140, S, X)
#ifdef VULKAN
#define gl_InstanceID  gl_InstanceIndex
#define gl_VertexID  gl_VertexIndex
#endif
