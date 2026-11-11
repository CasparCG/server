// Metal port of screen.vert / screen.frag (used by the macOS screen consumer).
#include <metal_stdlib>
using namespace metal;

#define COLOUR_SPACE_RGB                0
#define COLOUR_SPACE_DATAVIDEO_FULL     1
#define COLOUR_SPACE_DATAVIDEO_LIMITED  2

#define RANGE_16        (16.0f / 256.0f)
#define RANGE_235       (235.0f / 256.0f)
#define RANGE_HALF      (0.5f / 256.0f)
#define RANGE_LIMITED   ((235.0f - 16.0f) / 256.0f)

struct vertex_in
{
    float4 position; // xy used
    float4 tex_coord;
};

struct vertex_out
{
    float4 position [[position]];
    float4 tex_coord;
};

struct uniforms
{
    int key_only;
    int colour_space;
    int window_width;
    int padding;
};

vertex vertex_out screen_vertex(uint vid [[vertex_id]], const device vertex_in* vertices [[buffer(0)]])
{
    vertex_out out;
    out.position  = float4(vertices[vid].position.xy, 0.0f, 1.0f);
    out.tex_coord = vertices[vid].tex_coord;
    return out;
}

// rgb=0~255, y=16~235, uv=16~240 (column major, same as the GLSL mat3)
constant float3x3 rgb2yuv_709 = float3x3(float3(0.183f, -0.101f, 0.439f),
                                         float3(0.614f, -0.338f, -0.399f),
                                         float3(0.062f, 0.439f, -0.040f));

static float4 dtv_color(float4 color)
{
    color.xyz = rgb2yuv_709 * clamp(color.rgb / (color.a + 0.0000001f), 0.0f, 1.0f);
    return color;
}

fragment float4 screen_fragment(vertex_out in [[stage_in]],
                                texture2d<float> background [[texture(0)]],
                                sampler smp [[sampler(0)]],
                                constant uniforms& u [[buffer(0)]])
{
    float2 uv    = in.tex_coord.xy;
    float4 color = background.sample(smp, uv);

    if (u.key_only != 0) {
        color = float4(color.aaa, 1.0f);
    } else if (u.colour_space == COLOUR_SPACE_DATAVIDEO_FULL) { // Full range 0-255
        color             = dtv_color(color);
        float x_coord     = uv.x * u.window_width * 0.5f;
        bool  isEvenPixel = round(x_coord) - x_coord < 0.0f;
        float4 color2     = dtv_color(background.sample(
            smp, uv + (isEvenPixel ? float2(1.0f / u.window_width, 0.0f) : float2(-1.0f / u.window_width, 0.0f))));
        color.x = clamp((color.x * RANGE_LIMITED) + RANGE_16 + RANGE_HALF, RANGE_16, RANGE_235);
        color.y = clamp(((isEvenPixel ? color.y + color2.y : color.z + color2.z) * RANGE_LIMITED * 0.5f) + RANGE_HALF +
                            0.5f,
                        RANGE_16,
                        RANGE_235);
        color.z = clamp((color.w * RANGE_LIMITED) + RANGE_16 + RANGE_HALF, RANGE_16, RANGE_235);
    } else if (u.colour_space == COLOUR_SPACE_DATAVIDEO_LIMITED) { // Limited range 16-235
        color             = dtv_color(color);
        float x_coord     = uv.x * u.window_width * 0.5f;
        bool  isEvenPixel = round(x_coord) - x_coord < 0.0f;
        float4 color2     = dtv_color(background.sample(
            smp, uv + (isEvenPixel ? float2(1.0f / u.window_width, 0.0f) : float2(-1.0f / u.window_width, 0.0f))));
        color.x = clamp(color.x + RANGE_HALF, 0.0f, 1.0f);
        color.y = clamp(((isEvenPixel ? color.y + color2.y : color.z + color2.z) * 0.5f) + RANGE_HALF + 0.5f, 0.0f, 1.0f);
        color.z = clamp(color.w + RANGE_HALF, 0.0f, 1.0f);
    }
    return color;
}
