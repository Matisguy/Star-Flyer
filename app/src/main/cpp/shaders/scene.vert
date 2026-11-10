#version 450

// Resource sets follow SDL_CreateGPUShader's SPIR-V layout. Keep in step with the Metal source in
// Shader.cpp.
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;

layout(set = 1, binding = 0) uniform SceneVertexUniforms {
    mat4 uProjection;
};

void main() {
    fragUV = inUV;
    gl_Position = uProjection * vec4(inPosition, 1.0);
}
