/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Julian Waller, git@julusian.co.uk
*/

#include "screen_shader.h"

namespace caspar { namespace screen {

std::string get_vertex()
{
    return R"shader(
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
}

std::string get_fragment()
{
    return R"shader(
            #version 450
            uniform sampler2D background;

            in vec4 TexCoord;
            out vec4 fragColor;

			uniform bool key_only;

            void main() {
                vec4 color = texture(background, TexCoord.st);

                if (key_only)
                    color = vec4(color.aaa, 1);

                fragColor = color;
            }
	)shader";
}

std::unique_ptr<accelerator::ogl::shader> get_shader() {
    return std::make_unique<accelerator::ogl::shader>(get_vertex(), get_fragment());
}

}}
