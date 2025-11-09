#version 450

layout(location = 0) out vec4 ocolor;
layout(location = 0) in vec4 oPosition;

void main() {
    ocolor = oPosition;
}
