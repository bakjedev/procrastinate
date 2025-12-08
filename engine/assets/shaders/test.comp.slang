struct DrawIndexedIndirectCommand {
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
};

struct PushConstants {
    float input1;
    float input2;
};

[[vk::binding(0, 0)]]
RWStructuredBuffer<DrawIndexedIndirectCommand> outputCommands;

[shader("compute")]
[numthreads(1, 1, 1)]
void computeMain(uniform [[vk::push_constant]] PushConstants pc)
{
    outputCommands[0].indexCount = 372;
    outputCommands[0].instanceCount = 1;
    outputCommands[0].firstIndex = 0;
    outputCommands[0].vertexOffset = 0;
    outputCommands[0].firstInstance = 0;
    float result = pc.input1 + pc.input2;
}
