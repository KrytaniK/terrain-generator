#version 450

layout(location = 0) in vec3 world_pos;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform DebugGridConfig {
	float line_width;
	float cell_size;
	float anti_aliasing;
} config;

void main() {
	vec2 p = world_pos.xy;
	vec2 g = 0.5 * abs(fract(p)) / fwidth(p);
	float a = min(min(g.x, g.y), 2.0);

	vec4 line_color = vec4(0.5, 0.5, 0.5, 1.0 - a);

	if (abs(world_pos.x) < 0.02) {
		line_color.r = 1.0;
		line_color.g = 0.0;
		line_color.b = 0.0;
	} else if (abs(world_pos.y) < 0.02) {
		line_color.r = 0.0;
		line_color.g = 1.0;
		line_color.b = 0.0;
	}


	vec4 cell_color = vec4(0.0);

	// Assign color based on world position
	out_color = mix(line_color, cell_color, a);
} 