#version 450

layout(location = 0) out vec2 world_pos;
layout(location = 1) out vec2 cam_pos;
layout(location = 2) out float grid_scale;

layout(binding = 0) uniform ModelViewProjection {
    mat4 model;
    mat4 view;
    mat4 projection;
} mvp;

vec3 grid_square[4] = vec3[](
	vec3(-1.0, -1.0, 0.0), // Bottom Left
	vec3(1.0, -1.0, 0.0), // Bottom Right
	vec3(1.0, 1.0, 0.0), // Top Right
	vec3(-1.0, 1.0, 0.0) // Top Left
);

const int indices[6] = int[6](0, 2, 1, 2, 0, 3);

void main()
{
	float scale = 200;
	vec3 pos = scale * grid_square[indices[gl_VertexIndex]];

	vec2 cam = inverse(mvp.view)[3].xy;

	// Lock quad to camera position
	pos.xy += cam;

	// Calculate vertex position (in clip space)
	gl_Position =  mvp.projection * mvp.view * mvp.model * vec4(pos, 1.0);

	// Output the vertex position (in world space), and the camera position
	world_pos = pos.xy;
	cam_pos = cam;
	grid_scale = scale;
}