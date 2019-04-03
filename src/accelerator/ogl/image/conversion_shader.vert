#version 450
in vec4 TexCoordIn;
in vec2 Position;

out vec4 TexCoord;

void main()
{
    TexCoord = TexCoordIn;
    vec4 pos = vec4(Position, 0, 1);
    pos.x = pos.x*2.0 - 1.0;
    pos.y = pos.y*2.0 - 1.0;
    gl_Position = pos;
}
