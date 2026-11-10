#version 450

// Resource sets follow SDL_CreateGPUShader's SPIR-V layout. Keep in step with the Metal source in
// Shader.cpp.
layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D uTexture;

layout(set = 3, binding = 0) uniform SceneFragmentUniforms {
    vec4 uColor;
};

void main() {
    outColor = texture(uTexture, fragUV) * uColor;
}
