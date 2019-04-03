#version 450
in vec4 TexCoord;

out vec4 fragColor;

uniform sampler2D	plane[4];

uniform bool		is_hd;
uniform int			pixel_format;

uniform int 		texture_width;

vec4 ycbcra_to_rgba_sd(float Y, float Cb, float Cr, float A)
{
    vec4 rgba;
    rgba.b = (1.164*(Y*255 - 16) + 1.596*(Cr*255 - 128))/255;
    rgba.g = (1.164*(Y*255 - 16) - 0.813*(Cr*255 - 128) - 0.391*(Cb*255 - 128))/255;
    rgba.r = (1.164*(Y*255 - 16) + 2.018*(Cb*255 - 128))/255;
    rgba.a = A;
    return rgba;
}

vec4 ycbcra_to_rgba_hd(float Y, float Cb, float Cr, float A)
{
    vec4 rgba;
    rgba.b = (1.164*(Y*255 - 16) + 1.793*(Cr*255 - 128))/255;
    rgba.g = (1.164*(Y*255 - 16) - 0.534*(Cr*255 - 128) - 0.213*(Cb*255 - 128))/255;
    rgba.r = (1.164*(Y*255 - 16) + 2.115*(Cb*255 - 128))/255;
    rgba.a = A;
    return rgba;
}

vec4 ycbcra_to_rgba(float y, float cb, float cr, float a)
{
    if(is_hd)
        return ycbcra_to_rgba_hd(y, cb, cr, a);
    else
        return ycbcra_to_rgba_sd(y, cb, cr, a);
}

vec4 get_sample(sampler2D sampler, vec2 coords)
{
    return texture2D(sampler, coords);
}

vec4 get_rgba_color()
{
    switch(pixel_format)
    {
    case 10:	//uyvy,
        {
            float y = get_sample(plane[0], TexCoord.st).g;
            float cb;
            float cr;
            if(mod(TexCoord.s*texture_width, 2.0) < 1.0)
            {
                cb = get_sample(plane[0], TexCoord.st).r;
	            cr = textureOffset(plane[0], TexCoord.st, ivec2(1, 0)).r;
            }
            else
            {
	            cb = textureOffset(plane[0], TexCoord.st, ivec2(-1, 0)).r;
	            cr = get_sample(plane[0], TexCoord.st).r;
            }
            return ycbcra_to_rgba(y, cb, cr, 1.0);
        }
    }
    return vec4(0.0, 0.0, 0.0, 0.0);
}

void main()
{
    vec4 color = get_rgba_color();
    fragColor = color.bgra;
}
