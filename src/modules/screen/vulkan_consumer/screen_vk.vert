#version 450

// Push constants for screen rendering
layout(push_constant) uniform PushConstants {
    vec2 pos_scale;      // Position scale for aspect ratio
    vec2 pos_offset;     // Position offset
    vec2 tex_scale;      // Texture coordinate scale
    vec2 tex_offset;     // Texture coordinate offset
    int  key_only;       // Show alpha channel only
    int  colour_space;   // 0=RGB, 1=datavideo_full, 2=datavideo_limited
    int  window_width;   // Window width for DataVideo conversion
    int  _pad;           // Padding
} pc;

// Vertex output
layout(location = 0) out vec2 out_tex_coord;

// Full-screen quad vertices (two triangles)
// Vulkan clip space: Y=-1 is TOP, Y=+1 is BOTTOM
vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),   // top-left
    vec2( 1.0, -1.0),   // top-right
    vec2( 1.0,  1.0),   // bottom-right
    vec2(-1.0, -1.0),   // top-left
    vec2( 1.0,  1.0),   // bottom-right
    vec2(-1.0,  1.0)    // bottom-left
);

// Texture coordinates: match screen positions directly (no flip)
// Frame data is already in correct top-to-bottom orientation
vec2 tex_coords[6] = vec2[](
    vec2(0.0, 0.0),     // top-left
    vec2(1.0, 0.0),     // top-right
    vec2(1.0, 1.0),     // bottom-right
    vec2(0.0, 0.0),     // top-left
    vec2(1.0, 1.0),     // bottom-right
    vec2(0.0, 1.0)      // bottom-left
);

void main()
{
    vec2 pos = positions[gl_VertexIndex];
    vec2 tex = tex_coords[gl_VertexIndex];

    // Apply aspect ratio correction
    pos = pos * pc.pos_scale + pc.pos_offset;
    tex = tex * pc.tex_scale + pc.tex_offset;

    gl_Position = vec4(pos, 0.0, 1.0);
    out_tex_coord = tex;
}
