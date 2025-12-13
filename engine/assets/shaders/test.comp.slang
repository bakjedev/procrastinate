#include "shared.slangh"

[[vk::binding(0, 0)]]
RWStructuredBuffer<DrawIndexedIndirectCommand> outputCommands;

[[vk::binding(1, 0)]]
RWStructuredBuffer<Object> objects;

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;
    uint count, stride;
    objects.GetDimensions(count, stride);

    if (index >= count)
        return;

    Object obj = objects[index];
    
    outputCommands[index].indexCount = obj.indexCount;
    outputCommands[index].instanceCount = obj.instanceCount;
    outputCommands[index].firstIndex = obj.firstIndex;
    outputCommands[index].vertexOffset = obj.vertexOffset;
    outputCommands[index].firstInstance = obj.firstInstance;
}
