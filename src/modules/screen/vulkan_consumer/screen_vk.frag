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

// Texture sampler
layout(binding = 0) uniform sampler2D tex_background;

// Input from vertex shader
layout(location = 0) in vec2 in_tex_coord;

// Output color
layout(location = 0) out vec4 out_color;

// Constants for DataVideo conversion
#define COLOUR_SPACE_RGB                0
#define COLOUR_SPACE_DATAVIDEO_FULL     1
#define COLOUR_SPACE_DATAVIDEO_LIMITED  2

#define RANGE_16        (16.0 / 256.0)
#define RANGE_235       (235.0 / 256.0)
#define RANGE_HALF      (0.5 / 256.0)
#define RANGE_LIMITED   ((235.0 - 16.0) / 256.0)

// RGB to YUV conversion matrix (BT.709)
// rgb=0~255, y=16~235, uv=16~240
const mat3 rgb2yuv_709 = mat3(
    0.183,  -0.101,  0.439,
    0.614,  -0.338, -0.399,
    0.062,   0.439, -0.040
);

// Sample texture and convert BGRA to RGBA
// Frame data is in BGRA format (CasparCG standard) but stored in R8G8B8A8 texture,
// so when sampled we get (B, G, R, A) - need to swizzle to (R, G, B, A)
vec4 sample_texture(vec2 coord)
{
    vec4 texel = texture(tex_background, coord);
    return texel.bgra;  // Swizzle BGRA -> RGBA
}

vec4 dtv_color(vec4 color)
{
    color.stp = rgb2yuv_709 * clamp(color.rgb / (color.a + 0.0000001), 0.0, 1.0);
    return color;
}

void main()
{
    vec4 color = sample_texture(in_tex_coord);

    if (pc.key_only == 1) {
        // Show alpha channel as grayscale
        color = vec4(color.aaa, 1.0);
    }
    else if (pc.colour_space == COLOUR_SPACE_DATAVIDEO_FULL) {
        // Full range 0-255
        color = dtv_color(color);
        float x_coord = in_tex_coord.x * float(pc.window_width) * 0.5;
        bool isEvenPixel = round(x_coord) - x_coord < 0.0;
        vec2 offset = isEvenPixel ? vec2(1.0 / float(pc.window_width), 0.0) : vec2(-1.0 / float(pc.window_width), 0.0);
        vec4 color2 = dtv_color(sample_texture(in_tex_coord + offset));
        color.s = clamp((color.s * RANGE_LIMITED) + RANGE_16 + RANGE_HALF, RANGE_16, RANGE_235);
        color.t = clamp(((isEvenPixel ? color.t + color2.t : color.p + color2.p) * RANGE_LIMITED * 0.5) + RANGE_HALF + 0.5, RANGE_16, RANGE_235);
        color.p = clamp((color.w * RANGE_LIMITED) + RANGE_16 + RANGE_HALF, RANGE_16, RANGE_235);
    }
    else if (pc.colour_space == COLOUR_SPACE_DATAVIDEO_LIMITED) {
        // Limited range 16-235
        color = dtv_color(color);
        float x_coord = in_tex_coord.x * float(pc.window_width) * 0.5;
        bool isEvenPixel = round(x_coord) - x_coord < 0.0;
        vec2 offset = isEvenPixel ? vec2(1.0 / float(pc.window_width), 0.0) : vec2(-1.0 / float(pc.window_width), 0.0);
        vec4 color2 = dtv_color(sample_texture(in_tex_coord + offset));
        color.s = clamp(color.s + RANGE_HALF, 0.0, 1.0);
        color.t = clamp(((isEvenPixel ? color.t + color2.t : color.p + color2.p) * 0.5) + RANGE_HALF + 0.5, 0.0, 1.0);
        color.p = clamp(color.w + RANGE_HALF, 0.0, 1.0);
    }

    // Output to swapchain - Vulkan handles B8G8R8A8 format conversion automatically
    out_color = vec4(color.rgb, 1.0);
}
