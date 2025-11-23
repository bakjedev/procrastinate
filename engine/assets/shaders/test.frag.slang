#include "shared.slangh"

[shader("fragment")]
float3 main(VertexOutput input) : SV_Target
{
	float2 uv = input.position.xy;
	float step = 20.0;
	float2 snapped = round(uv / step) * step;
	float2 grid = frac(snapped * 0.11);
	return float3(grid, 1.0);
}
