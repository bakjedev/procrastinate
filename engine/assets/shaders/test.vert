#version 450

const vec2 positions[3] = vec2[](
    vec2(-0.5, 0.5),
    vec2( 0.5, 0.5),
    vec2( 0.0, -0.5)
);

layout(location = 0) out vec4 oPosition;


void main() {
	oPosition = vec4(positions[gl_VertexIndex], 1.0, 1.0);
    gl_Position = oPosition;
}