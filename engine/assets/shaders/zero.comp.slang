[[vk::binding(2, 0)]]
RWStructuredBuffer<uint> drawCount;

[shader("compute")]
[numthreads(1, 1, 1)]
void main()
{
    drawCount[0] = 0;
}