#include "shared.slangh"

struct VertexInput
{
    float3 position : POSITION;
};

[shader("vertex")]
VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.color = float4(input.position, 1.0);
    output.position = output.color;
    return output;
}