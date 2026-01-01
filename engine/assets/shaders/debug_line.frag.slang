#include "shared.slangh"

[shader("fragment")]
float3 main(DebugLineVertexOutput input) : SV_Target
{
    float3 color = input.color;

    return color;
}
