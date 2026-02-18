#include "shared.slangh"

[shader("fragment")]
uint main(VertexOutput input) : SV_Target
{
	return input.objectID;
}
