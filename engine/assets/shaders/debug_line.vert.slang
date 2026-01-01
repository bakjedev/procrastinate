#include "shared.slangh"

struct VertexInput
{
    float3 position : POSITION;
    float3 color : COLOR0;
};

[[vk::push_constant]]
PushConstants pushConst;


[shader("vertex")]
DebugLineVertexOutput main(VertexInput input)
{
    DebugLineVertexOutput output;

    float4 worldPos = float4(input.position, 1.0);   
    float4 viewPos = mul(pushConst.view, worldPos);    

    output.position = mul(pushConst.proj, viewPos);
    output.color = input.color;
    return output;
}
