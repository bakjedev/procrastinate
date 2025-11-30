#version 450

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(push_constant) uniform PushConstants {
  float input1;
  float input2;
} pc;

void main() {
  float result = pc.input1 + pc.input2;
}
