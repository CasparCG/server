#version 450
in vec4 TexCoordIn;
in vec2 Position;

out vec4 TexCoord;

void main()
{
    TexCoord = TexCoordIn;
    gl_Position = vec4(Position, 0, 1);
}
