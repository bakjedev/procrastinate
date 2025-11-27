#include "shared.slangh"

[shader("fragment")]
float3 main(VertexOutput input) : SV_Target
{
	return input.color;
}
