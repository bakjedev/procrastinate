#include "shared.slangh"

[shader("fragment")]
float3 main(VertexOutput input) : SV_Target
{
    float3 color = input.color;

    float cosTheta = dot(input.normal, float3(0.3, -0.6, 0.1));

    color *= max(0.1, cosTheta);

	return color;
}
