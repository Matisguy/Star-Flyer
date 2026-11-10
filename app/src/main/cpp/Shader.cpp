#include "Shader.h"

#include <cstddef>
#include <cstring>

#include "Model.h"

namespace {

#ifdef SDL_PLATFORM_IOS

// Metal compiles shader source at load time. Keep in step with shaders/scene.vert and scene.frag.
const char kVertexCode[] = R"msl(
#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float2 uv [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

struct SceneVertexUniforms {
    float4x4 projection;
};

vertex VertexOut scene_vertex(VertexIn in [[stage_in]],
                              constant SceneVertexUniforms &uniforms [[buffer(0)]]) {
    VertexOut out;
    out.position = uniforms.projection * float4(in.position, 1.0);
    out.uv = in.uv;
    return out;
}
)msl";

const char kFragmentCode[] = R"msl(
#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

struct SceneFragmentUniforms {
    float4 color;
};

fragment float4 scene_fragment(VertexOut in [[stage_in]],
                               texture2d<float> sceneTexture [[texture(0)]],
                               sampler sceneSampler [[sampler(0)]],
                               constant SceneFragmentUniforms &uniforms [[buffer(0)]]) {
    return sceneTexture.sample(sceneSampler, in.uv) * uniforms.color;
}
)msl";

const char *const kVertexEntry = "scene_vertex";
const char *const kFragmentEntry = "scene_fragment";
const size_t kVertexCodeSize = sizeof(kVertexCode) - 1;
const size_t kFragmentCodeSize = sizeof(kFragmentCode) - 1;

#else

// Generated at build time from shaders/ by the NDK's glslc (see CMakeLists.txt).
const Uint32 kVertexCode[] = {
#include "scene.vert.spv.inc"
};

const Uint32 kFragmentCode[] = {
#include "scene.frag.spv.inc"
};

const char *const kVertexEntry = "main";
const char *const kFragmentEntry = "main";
const size_t kVertexCodeSize = sizeof(kVertexCode);
const size_t kFragmentCodeSize = sizeof(kFragmentCode);

#endif

} // namespace

SDL_GPUShaderFormat Shader::shaderFormat() {
#ifdef SDL_PLATFORM_IOS
    return SDL_GPU_SHADERFORMAT_MSL;
#else
    return SDL_GPU_SHADERFORMAT_SPIRV;
#endif
}

std::unique_ptr<Shader> Shader::create(
        SDL_GPUDevice *device,
        SDL_GPUTextureFormat colorFormat,
        SDL_GPUTextureFormat depthFormat) {
    SDL_GPUShaderCreateInfo vertexInfo = {};
    vertexInfo.code = reinterpret_cast<const Uint8 *>(kVertexCode);
    vertexInfo.code_size = kVertexCodeSize;
    vertexInfo.entrypoint = kVertexEntry;
    vertexInfo.format = shaderFormat();
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertexInfo.num_uniform_buffers = 1;

    SDL_GPUShaderCreateInfo fragmentInfo = {};
    fragmentInfo.code = reinterpret_cast<const Uint8 *>(kFragmentCode);
    fragmentInfo.code_size = kFragmentCodeSize;
    fragmentInfo.entrypoint = kFragmentEntry;
    fragmentInfo.format = shaderFormat();
    fragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragmentInfo.num_samplers = 1;
    fragmentInfo.num_uniform_buffers = 1;

    SDL_GPUShader *vertexShader = SDL_CreateGPUShader(device, &vertexInfo);
    if (!vertexShader) {
        return nullptr;
    }
    SDL_GPUShader *fragmentShader = SDL_CreateGPUShader(device, &fragmentInfo);
    if (!fragmentShader) {
        SDL_ReleaseGPUShader(device, vertexShader);
        return nullptr;
    }

    SDL_GPUVertexBufferDescription vertexBuffer = {};
    vertexBuffer.slot = 0;
    vertexBuffer.pitch = sizeof(Vertex);
    vertexBuffer.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute attributes[2] = {};
    attributes[0].location = 0;
    attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attributes[0].offset = offsetof(Vertex, position);
    attributes[1].location = 1;
    attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attributes[1].offset = offsetof(Vertex, uv);

    SDL_GPUColorTargetDescription colorTarget = {};
    colorTarget.format = colorFormat;

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &vertexBuffer;
    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_attributes = attributes;
    pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipelineInfo.rasterizer_state.enable_depth_clip = true;
    pipelineInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    pipelineInfo.depth_stencil_state.enable_depth_test = true;
    pipelineInfo.depth_stencil_state.enable_depth_write = true;
    pipelineInfo.target_info.color_target_descriptions = &colorTarget;
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.depth_stencil_format = depthFormat;
    pipelineInfo.target_info.has_depth_stencil_target = true;

    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_ReleaseGPUShader(device, fragmentShader);
    if (!pipeline) {
        return nullptr;
    }

    return std::unique_ptr<Shader>(new Shader(device, pipeline));
}

Shader::~Shader() {
    SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
}

void Shader::activate(SDL_GPUCommandBuffer *commandBuffer, SDL_GPURenderPass *renderPass) {
    commandBuffer_ = commandBuffer;
    renderPass_ = renderPass;
    SDL_BindGPUGraphicsPipeline(renderPass_, pipeline_);
}

void Shader::deactivate() {
    commandBuffer_ = nullptr;
    renderPass_ = nullptr;
}

void Shader::drawModel(const Model &model) const {
    SDL_GPUBufferBinding vertexBinding = {model.getVertexBuffer(), 0};
    SDL_BindGPUVertexBuffers(renderPass_, 0, &vertexBinding, 1);

    SDL_GPUBufferBinding indexBinding = {model.getIndexBuffer(), 0};
    SDL_BindGPUIndexBuffer(renderPass_, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_GPUTextureSamplerBinding textureBinding = {
            model.getTexture().getTexture(),
            model.getTexture().getSampler()};
    SDL_BindGPUFragmentSamplers(renderPass_, 0, &textureBinding, 1);

    SDL_DrawGPUIndexedPrimitives(renderPass_, model.getIndexCount(), 1, 0, 0, 0);
}

void Shader::setProjectionMatrix(const float *projectionMatrix) const {
    SDL_PushGPUVertexUniformData(commandBuffer_, 0, projectionMatrix, sizeof(float) * 16);
}

void Shader::setColor(float r, float g, float b, float a) const {
    const float color[4] = {r, g, b, a};
    SDL_PushGPUFragmentUniformData(commandBuffer_, 0, color, sizeof(color));
}
