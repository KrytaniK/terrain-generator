#version 450

layout(location = 0) in vec2 world_pos;
layout(location = 1) in vec2 cam_pos;
layout(location = 2) in float grid_scale;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform DebugGridConfig {
	float line_width;
	float cell_size;
	float anti_aliasing;
} config;

void main() {
	// Grid-Space UV
	vec2 uv = world_pos.xy;

	// Compute Grid Line
	vec2 grid = 0.5 *  abs(fract(uv)) / fwidth(uv);
	float line = min(grid.x, grid.y);

	// Apply some smoothing to the overall line thickness (Anti-Aliasing)
	vec2 dist_to_line = abs(fract(uv) - 0.5);
	vec2 smoothing = smoothstep(vec2(0.5), 0.5 - fwidth(uv) * 2.0, dist_to_line);

	// Line thickness smoothing
	float thickness = 1.0 - min(smoothing.x, smoothing.y);

	// Distance-based fading
	float dist = length(uv - cam_pos);
	float fade = 1.0 - smoothstep(0.0, grid_scale * 0.9, dist);

	// Final grid value
	float alpha = thickness * fade;

	// If the resulting alpha value is 0, discard the fragment
	if (alpha == 0.0)
		discard;

	// Base Color
	vec4 line_color = vec4(0.35, 0.35, 0.35, alpha);

	// Axis Colors
	vec4 x_axis_color = vec4(1.0, 0.0, 0.0, 1.0);
	vec4 y_axis_color = vec4(0.0, 1.0, 0.0, 1.0);

	vec4 final_color = line_color;

	// Blend factors for x/y axis lines.
	float x_blend_factor = 1.0 - smoothstep(0.0, alpha, length(uv.y) + smoothing.y);
	float y_blend_factor = 1.0 - smoothstep(0.0, alpha, length(uv.x) + smoothing.x);

	final_color = mix(line_color, x_axis_color, x_blend_factor);
	final_color = mix(final_color, y_axis_color, y_blend_factor);

	out_color = mix(vec4(0.0), final_color, alpha);
} 