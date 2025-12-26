#include "shared.slangh"

struct VertexInput
{
    float3 position : POSITION;
    float3 color : COLOR0;
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

float3 hsv2rgb(float3 c) {
    float4 K = float4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

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

    float maxDistance = 900.0f;
    float hue = float(index) / maxDistance;
    output.color = hsv2rgb(float3(hue, 0.6, 0.8));
    return output;
}
