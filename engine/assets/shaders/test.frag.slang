#include "shared.slangh"

[shader("fragment")]
uint main(VertexOutput input, uint primitiveID : SV_PrimitiveID) : SV_Target
{
    uint packed = (primitiveID << 16) | (input.drawID & 0xFFFF);
	return packed;
}
