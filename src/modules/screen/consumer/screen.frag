#version 450

#define COLOUR_SPACE_RGB                0
#define COLOUR_SPACE_DATAVIDEO_FULL     1
#define COLOUR_SPACE_DATAVIDEO_LIMITED  2

#define RANGE_16        (16.0f / 256.0f)
#define RANGE_235       (235.0f / 256.0f)
#define RANGE_HALF      (0.5f / 256.0f)
#define RANGE_LIMITED   ((235.0f - 16.0f) / 256.0f)

uniform sampler2D background;

in vec4 TexCoord;
out vec4 fragColor;

uniform bool key_only;
uniform int colour_space;
uniform int window_width;

// rgb=0~255, y=16~235, uv=16~240
mat3 rgb2yuv_709 = mat3(0.183f, -0.101f, 0.439f,  0.614f, -0.338f, -0.399f, 0.062f, 0.439f,-0.040f);

vec4 dtv_color(vec4 color)
{
    color.stp = rgb2yuv_709 * clamp(color.rgb  / (color.a + 0.0000001f), 0.0f, 1.0f);
    return color;
}

void main()
{
    vec4 color = texture(background, TexCoord.xy);
    if (key_only)
        color = vec4(color.aaa, 1);

    else if (colour_space == COLOUR_SPACE_DATAVIDEO_FULL) {  // Full range 0-255
        color = dtv_color(color);
        float x_coord = TexCoord.x * window_width * 0.5f;
        bool isEvenPixel = round(x_coord) - x_coord < 0.0f ;
        vec4 color2 = dtv_color(texture(background, TexCoord.xy + (isEvenPixel ? vec2(1.0f / window_width, 0.0f) : vec2(-1.0f / window_width, 0.0f))));
        color.s = clamp((color.s * RANGE_LIMITED) + RANGE_16 + RANGE_HALF, RANGE_16, RANGE_235);
        color.t = clamp(((isEvenPixel ? color.t + color2.t : color.p + color2.p) * RANGE_LIMITED * 0.5f) + RANGE_HALF + 0.5f, RANGE_16, RANGE_235);
        color.p = clamp((color.w * RANGE_LIMITED) + RANGE_16 + RANGE_HALF, RANGE_16, RANGE_235);

    } else if (colour_space == COLOUR_SPACE_DATAVIDEO_LIMITED) {  // Limited range 16-235
        color = dtv_color(color);
        float x_coord = TexCoord.x * window_width * 0.5f;
        bool isEvenPixel = round(x_coord) - x_coord < 0.0f ;
        vec4 color2 = dtv_color(texture(background, TexCoord.xy + (isEvenPixel ? vec2(1.0f / window_width, 0.0f) : vec2(-1.0f / window_width, 0.0f))));
        color.s = clamp(color.s + RANGE_HALF, 0.0f, 1.0f);
        color.t = clamp(((isEvenPixel ? color.t + color2.t : color.p + color2.p) * 0.5f) + RANGE_HALF + 0.5f, 0.0f, 1.0f);
        color.p = clamp(color.w + RANGE_HALF, 0.0f, 1.0f);
    }
    fragColor = color;
}
