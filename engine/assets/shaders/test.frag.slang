#include "shared.slangh"

[shader("fragment")]
uint main(VertexOutput input, uint primitiveID : SV_PrimitiveID) : SV_Target
{
    uint packed = (input.objectID << 16) | (primitiveID & 0xFFFF);
	return packed;
}
