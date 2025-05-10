#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

layout(binding = 0) uniform ModelViewProjection {
    mat4 model;
    mat4 view;
    mat4 projection;
} mvp;

layout(location = 0) out vec3 frag_color;

void main() {
	gl_Position = mvp.projection * mvp.view * mvp.model * vec4(in_position, 1.0);
    gl_PointSize = 3.0;
	frag_color = in_color;
}