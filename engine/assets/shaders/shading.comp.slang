#include "shared.slangh"

[[vk::binding(1, 1)]]
StructuredBuffer<RenderObject> renderObjects;

[[vk::binding(3, 1)]]
Sampler2D<uint> visibilityImage;

[[vk::binding(4, 1)]]
RWTexture2D<float4> renderImage;

[[vk::binding(0, 0)]]
StructuredBuffer<MeshInfo> meshInfos;

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixel = uint2(dispatchThreadID.xy);

    uint packed = visibilityImage.Load(int3(pixel.x, pixel.y, 0));
    uint objectID = packed >> 16;
    uint primitiveID = packed & 0xFFFF;

    uint h = (objectID * 2654435761u) ^ (primitiveID * 2246822519u);
    float r = ((h >>  0) & 0xFF) / 255.0f;
    float g = ((h >>  8) & 0xFF) / 255.0f;
    float b = ((h >> 16) & 0xFF) / 255.0f;

    renderImage[pixel] = float4(r, g, b, 1);
}
