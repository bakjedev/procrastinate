struct PushConstants {
    float input1;
    float input2;
};

[shader("compute")]
[numthreads(1, 1, 1)]
void computeMain(uniform [[vk::push_constant]] PushConstants pc)
{
    float result = pc.input1 + pc.input2;
}
