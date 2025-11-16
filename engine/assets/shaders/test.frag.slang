#include "shared.slangh"

[shader("fragment")]
float4 main(VertexOutput input) : SV_Target
{
    return input.color;
}
