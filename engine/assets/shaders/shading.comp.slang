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

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixel = uint2(dispatchThreadID.xy);

    uint packed = visibilityImage.Load(int3(pixel.x, pixel.y, 0));
    uint objectID = packed >> 16;
    uint primitiveID = packed & 0xFFFF;

    RenderObject object = renderObjects[objectID];
    MeshInfo mesh = meshInfos[object.meshID];

    uint triangle_0 = indexBuffer[mesh.firstIndex + primitiveID * 3];
    VertexInputShading vertex = vertexBuffer[mesh.vertexOffset + triangle_0];

    if (all(abs(vertex.color - float3(1.0, 1.0, 1.0)) < float3(0.001))) {
        uint h = (objectID * 2654435761u) ^ (primitiveID * 2246822519u);
        float r = ((h >>  0) & 0xFF) / 255.0f;
        float g = ((h >>  8) & 0xFF) / 255.0f;
        float b = ((h >> 16) & 0xFF) / 255.0f;

        renderImage[pixel] = float4(r, g, b, 1);
    } else {
        renderImage[pixel] = float4(vertex.color, 1.0);
    }
}
