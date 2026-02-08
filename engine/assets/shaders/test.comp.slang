#include "shared.slangh"

[[vk::binding(0, 1)]]
RWStructuredBuffer<DrawIndexedIndirectCommand> outputCommands;

[[vk::binding(1, 1)]]
StructuredBuffer<RenderObject> renderObjects;

[[vk::binding(0, 0)]]
StructuredBuffer<MeshInfo> meshInfos;

struct ComputePushConstants
{
    Frustum frustum;
};

[[vk::push_constant]]
ComputePushConstants pushConst;

[[vk::binding(2, 1)]]
RWStructuredBuffer<uint> drawCount;

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 gtid : SV_GroupThreadID)
{
    uint index = dispatchThreadID.x;
    uint count, stride;
    renderObjects.GetDimensions(count, stride);

    if (index >= count)
        return;

    RenderObject obj = renderObjects[index];
    MeshInfo info = meshInfos[obj.meshID];

    uint countedIndex;
    InterlockedAdd(drawCount[0], 1, countedIndex);
    
    outputCommands[countedIndex].indexCount = info.indexCount;
    outputCommands[countedIndex].instanceCount = 1;
    outputCommands[countedIndex].firstIndex = info.firstIndex;
    outputCommands[countedIndex].vertexOffset = info.vertexOffset;
    outputCommands[countedIndex].firstInstance = index;
}
