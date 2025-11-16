struct VertexInput
{
    float3 position : POSITION;
};

// Vertex to Fragment data
struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

// Vertex Shader
[shader("vertex")]
VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.color = float4(input.position, 1.0);
    output.position = output.color;
    return output;
}