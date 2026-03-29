#include "shared.slangh"

[[vk::binding(1, 0)]]
Sampler2D textures[];

[[vk::binding(5, 1)]]
RWTexture2D<uint> overdraw;


[[vk::binding(3, 0)]]
ConstantBuffer<UniformBuffer> ubo;

[shader("fragment")]
float3 main(VertexOutput input, float4 pos : SV_Position) : SV_Target
{
	float3 color = input.color;

    if (input.texture_id >= 0) {
	    float3 texture_color = textures[input.texture_id].Sample(input.tex_coord).xyz;
        color *= texture_color;
    }
    float cosTheta = dot(input.normal, float3(0.3, -0.6, 0.1));

    color *= max(0.3, cosTheta);

    InterlockedAdd(overdraw[uint2(pos.xy)], 1);

    return color;
}
