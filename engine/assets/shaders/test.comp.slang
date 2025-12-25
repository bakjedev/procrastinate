#include "shared.slangh"

[[vk::binding(0, 1)]]
RWStructuredBuffer<DrawIndexedIndirectCommand> outputCommands;

[[vk::binding(1, 1)]]
StructuredBuffer<RenderObject> renderObjects;

[[vk::binding(0, 0)]]
StructuredBuffer<MeshInfo> meshInfos;

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(
uint3 dispatchThreadID : SV_DispatchThreadID,
uint baseInstance : SV_StartInstanceLocation
)
{
    uint index = dispatchThreadID.x;
    uint count, stride;
    renderObjects.GetDimensions(count, stride);

    if (index >= count)
        return;

    RenderObject obj = renderObjects[index];
    MeshInfo info = meshInfos[obj.meshID];

    outputCommands[index].indexCount = info.indexCount;
    outputCommands[index].instanceCount = 1;
    outputCommands[index].firstIndex = info.firstIndex;
    outputCommands[index].vertexOffset = info.vertexOffset;
    outputCommands[index].firstInstance = index;
}
