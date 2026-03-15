#include "shared.slangh"

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

[[vk::push_constant]]
PushConstants pushConst;

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

    uint objectID = packed >> 16;
    uint primitiveID = packed & 0xFFFF;

    RenderObject object = renderObjects[objectID];
    MeshInfo mesh = meshInfos[object.meshID];

    uint triangle_0 = indexBuffer[mesh.firstIndex + primitiveID * 3 + 0];
    uint triangle_1 = indexBuffer[mesh.firstIndex + primitiveID * 3 + 1];
    uint triangle_2 = indexBuffer[mesh.firstIndex + primitiveID * 3 + 2];

    VertexInputShading vertex_0 = vertexBuffer[mesh.vertexOffset + triangle_0];
    VertexInputShading vertex_1 = vertexBuffer[mesh.vertexOffset + triangle_1];
    VertexInputShading vertex_2 = vertexBuffer[mesh.vertexOffset + triangle_2];

    float3 color = (vertex_0.color + vertex_1.color + vertex_2.color) / 3.0;

    renderImage[pixel] = float4(color, 1.0);
}
