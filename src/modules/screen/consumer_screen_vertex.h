#pragma once

namespace caspar { namespace screen {

// Vertex shader for screen consumer
// Simple pass-through vertex shader with texture coordinates
static const char* vertex_shader = R"shader(
#version 450
in vec4 TexCoordIn;
in vec2 Position;

out vec4 TexCoord;

void main()
{
    TexCoord = TexCoordIn;
    gl_Position = vec4(Position, 0, 1);
}
)shader";

}} // namespace caspar::screen
