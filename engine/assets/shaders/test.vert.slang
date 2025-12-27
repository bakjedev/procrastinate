#include "shared.slangh"

struct VertexInput
{
    float3 position : POSITION;
    float3 color : COLOR0;
    float3 normal : NORMAL;
};

struct PushConstants
{
    float4x4 view;
    float4x4 proj;
};

[[vk::push_constant]]
PushConstants pushConst;

[[vk::binding(1, 0)]]
StructuredBuffer<RenderObject> renderObjects;

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
    output.color = input.color;
    output.normal = input.normal;
    return output;
}
