#version 450

const vec2 positions[3] = vec2[](
    vec2(-0.5, 0.5),  // Bottom-left
    vec2( 0.5, 0.5),  // Bottom-right
    vec2( 0.0, -0.5)  // Top-right
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}