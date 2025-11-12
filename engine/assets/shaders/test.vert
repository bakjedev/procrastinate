#version 450

layout(location = 0) in vec3 aPosition;

const vec2 positions[3] = vec2[](
    vec2(-0.5, 0.5),
    vec2( 0.5, 0.5),
    vec2( 0.0, -0.5)
);

layout(location = 0) out vec4 oPosition;


void main() {
	oPosition = vec4(aPosition, 1.0);
    gl_Position = oPosition;
}