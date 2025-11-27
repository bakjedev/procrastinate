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

[shader("vertex")]
VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.color = input.color;
    output.position = mul(mul(pushConst.proj, pushConst.view), float4(input.position, 1.0));
    return output;
}