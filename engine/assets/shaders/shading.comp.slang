#include "shared.slangh"

[[vk::binding(0, 1)]]
RWStructuredBuffer<DrawIndexedIndirectCommand> outputCommands;

[[vk::binding(1, 1)]]
StructuredBuffer<RenderObject> renderObjects;

[[vk::binding(3, 1)]]
Sampler2D<uint> visibilityImage;

[[vk::binding(4, 1)]]
RWTexture2D<float4> renderImage;

[[vk::binding(0, 0)]]
StructuredBuffer<MeshInfo> meshInfos;

[[vk::binding(2, 0)]]
StructuredBuffer<VertexInputShading> vertexBuffer;

[[vk::binding(3, 0)]]
StructuredBuffer<uint> indexBuffer;

[[vk::binding(1, 0)]]
Sampler2D textures[];

[[vk::binding(4, 0)]]
ConstantBuffer<UniformBuffer> ubo;

[[vk::push_constant]]
PushConstants pushConst;

float cross2d(float2 u, float2 v) {
    return u.x * v.y - u.y * v.x;
}

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixel = uint2(dispatchThreadID.xy);

    uint packed = visibilityImage.Load(int3(pixel.x, pixel.y, 0));
    if (packed == 0xFFFFFFFFu) {
        renderImage[pixel] = float4(float3(0.0), 1.0);
        return;
    }

    uint primitiveID = packed >> 16;
    uint drawID = packed & 0xFFFF;
    DrawIndexedIndirectCommand command = outputCommands[drawID];
    RenderObject object = renderObjects[command.firstInstance];

    uint triangle_0 = indexBuffer[command.firstIndex + primitiveID * 3 + 0];
    uint triangle_1 = indexBuffer[command.firstIndex + primitiveID * 3 + 1];
    uint triangle_2 = indexBuffer[command.firstIndex + primitiveID * 3 + 2];

    VertexInputShading vertex_0 = vertexBuffer[command.vertexOffset + triangle_0];
    VertexInputShading vertex_1 = vertexBuffer[command.vertexOffset + triangle_1];
    VertexInputShading vertex_2 = vertexBuffer[command.vertexOffset + triangle_2];

    // calculate screen position of each triangle corner
    float4x4 mvp = mul(pushConst.proj, mul(pushConst.view, object.model));
    float4 clip_0 = mul(mvp, float4(vertex_0.position, 1.0));
    float4 clip_1 = mul(mvp, float4(vertex_1.position, 1.0));
    float4 clip_2 = mul(mvp, float4(vertex_2.position, 1.0));

    float3 ndc_0 = clip_0.xyz / clip_0.w;
    float3 ndc_1 = clip_1.xyz / clip_1.w;
    float3 ndc_2 = clip_2.xyz / clip_2.w;

    float2 screen_0 = (ndc_0.xy * 0.5 + 0.5) * ubo.viewport;
    float2 screen_1 = (ndc_1.xy * 0.5 + 0.5) * ubo.viewport;
    float2 screen_2 = (ndc_2.xy * 0.5 + 0.5) * ubo.viewport;

    float2 pixel_pos = (float2)pixel + 0.5;

    // calculate the areas of the full triangle and the 3 smaller triangles inside
    float2 ab = screen_1 - screen_0;
    float2 ac = screen_2 - screen_0;
    float abc = cross2d(ab, ac);

    float2 bc = screen_2 - screen_1;
    float2 bp = pixel_pos - screen_1;
    float bcp = cross2d(bc, bp);

    float2 ca = -ac;
    float2 cp = pixel_pos - screen_2;
    float cap = cross2d(ca, cp);

    // calculate the barycentric coords. Not taking in account the depth they have.
    float bary_0 = bcp / abc;
    float bary_1 = cap / abc;
    float bary_2 = 1.0 - bary_0 - bary_1;

    // Getting the weights based on depth. so the further something is the smaller the weight
    float inv_w0 = 1.0 / clip_0.w;
    float inv_w1 = 1.0 / clip_1.w;
    float inv_w2 = 1.0 / clip_2.w;

    // multiplying the barycentric coords by these weights. but now no longer add up to 1.
    float b0w = bary_0 * inv_w0;
    float b1w = bary_1 * inv_w1;
    float b2w = bary_2 * inv_w2;

    // get the value to multiply by to get it back to 1.
    float inv_denom = 1.0 / (b0w + b1w + b2w);

    // interpolate! first using the weighted barycentric coords and then multiplying to get it back to a sum of 1.
    float3 color = (vertex_0.color * b0w + vertex_1.color * b1w + vertex_2.color * b2w) * inv_denom;
    float2 uv = (vertex_0.tex_coord * b0w + vertex_1.tex_coord * b1w + vertex_2.tex_coord * b2w) * inv_denom;
    float3 normal = (vertex_0.normal * b0w + vertex_1.normal * b1w + vertex_2.normal * b2w) * inv_denom;

    if (object.textureID >= 0) {
	    float3 texture_color = textures[object.textureID].SampleLevel(uv, 0).xyz;
        color *= texture_color;
    }

    float cosTheta = dot(normal, float3(0.3, -0.6, 0.1));
    color *= max(0.3, cosTheta);

    renderImage[pixel] = float4(color, 1.0);
}
