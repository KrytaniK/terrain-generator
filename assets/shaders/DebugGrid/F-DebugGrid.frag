#version 450

layout(location = 0) in vec3 world_pos;
layout(location = 1) in float grid_scale;
layout(location = 2) in vec3 cam_pos;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform DebugGridConfig {
	float line_width;
	float cell_size;
	float anti_aliasing;
} config;

float checkerAA(vec2 uv, float scale) {
    vec2 coord = uv * scale;
    vec2 derivative = fwidth(coord);
    vec2 filtered = smoothstep(0.5 - derivative, 0.5 + derivative, fract(coord));
    return mix(1.0, 0.0, filtered.x * filtered.y + (1.0 - filtered.x) * (1.0 - filtered.y));
}

void main() {
	// Grid-Space UV
	vec2 uv = world_pos.xy;

	// Anti-aliasing using screen-space derivatives
	vec2 grid = 0.5 *  abs(fract(uv)) / fwidth(uv);
	float line = min(grid.x, grid.y);

	// Apply some smoothing
	vec2 dist_to_line = abs(fract(uv) - 0.5);
	vec2 smoothing = smoothstep(vec2(0.5), 0.5 - fwidth(uv) * 2.0, dist_to_line);

	// Line thickness control
	float thickness = 1.0 - min(smoothing.x, smoothing.y);

	// Distance-based fading
	float dist = length(uv);
	float fade = exp(-dist * 0.02); // exponential falloff

	// Final grid value
	float alpha = thickness * fade;

	vec4 cell_color = vec4(0.0, 0.0, 0.0, 0.0);
	vec4 line_color = vec4(0.5, 0.5, 0.5, 1.0);

	out_color = mix(cell_color, line_color, alpha);
} 