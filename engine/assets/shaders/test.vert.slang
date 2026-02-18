#include "shared.slangh"

struct VertexInput
{
    float3 position : POSITION;
};

[[vk::push_constant]]
PushConstants pushConst;

[[vk::binding(1, 1)]]
StructuredBuffer<RenderObject> renderObjects;

[[vk::binding(0, 0)]]
StructuredBuffer<MeshInfo> meshInfos;

[shader("vertex")]
VertexOutput main(VertexInput input, uint instanceID : SV_InstanceID, uint baseInstance : SV_StartInstanceLocation)
{
    VertexOutput output;

    uint index = baseInstance + instanceID;
    RenderObject renderObject = renderObjects[index];

    float4x4 model = renderObject.model;
    float4 worldPos = mul(model, float4(input.position, 1.0));   
    float4 viewPos = mul(pushConst.view, worldPos);    

    output.position = mul(pushConst.proj, viewPos);

    MeshInfo mesh = meshInfos[renderObject.meshID];
    uint primitive = mesh.firstIndex / 3;

    output.objectID = (index << 16) | (primitive & 0xFFFF);
    return output;
}
