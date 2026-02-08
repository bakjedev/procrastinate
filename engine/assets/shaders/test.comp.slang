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


bool isOnFrustum(Frustum frustum, MeshInfo info, float4x4 world) {
    const float3 corners[8] = {
        mul(world, float4(info.bmin.x, info.bmin.y, info.bmin.z, 1.0)).xyz,
        mul(world, float4(info.bmax.x, info.bmin.y, info.bmin.z, 1.0)).xyz,
        mul(world, float4(info.bmin.x, info.bmax.y, info.bmin.z, 1.0)).xyz,
        mul(world, float4(info.bmax.x, info.bmax.y, info.bmin.z, 1.0)).xyz,
        mul(world, float4(info.bmin.x, info.bmin.y, info.bmax.z, 1.0)).xyz,
        mul(world, float4(info.bmax.x, info.bmin.y, info.bmax.z, 1.0)).xyz,
        mul(world, float4(info.bmin.x, info.bmax.y, info.bmax.z, 1.0)).xyz,
        mul(world, float4(info.bmax.x, info.bmax.y, info.bmax.z, 1.0)).xyz,
    };

    Plane planes[6] = {frustum.left, frustum.right, frustum.bottom, frustum.top, frustum.near, frustum.top};

    for (int i = 0; i < 6; i++) {
        bool allOutside = true;

        for (int j = 0; j < 8; j++) {
            if (dot(planes[i].normal, corners[j]) + planes[i].distance >= -0.001) {
                allOutside = false;
                break;
            }
        }
        if (allOutside) {
            return false;
        }
    }
    return true;
}

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

    if (isOnFrustum(pushConst.frustum, info, obj.model)) {
        uint countedIndex;
        InterlockedAdd(drawCount[0], 1, countedIndex);

        outputCommands[countedIndex].indexCount = info.indexCount;
        outputCommands[countedIndex].instanceCount = 1;
        outputCommands[countedIndex].firstIndex = info.firstIndex;
        outputCommands[countedIndex].vertexOffset = info.vertexOffset;
        outputCommands[countedIndex].firstInstance = index;
    }
}
