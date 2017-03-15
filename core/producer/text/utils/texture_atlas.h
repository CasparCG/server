/* ============================================================================
 * Freetype GL - A C OpenGL Freetype engine
 * Platform:    Any
 * WWW:         http://code.google.com/p/freetype-gl/
 * ----------------------------------------------------------------------------
 * Copyright 2011,2012 Nicolas P. Rougier. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NICOLAS P. ROUGIER ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL NICOLAS P. ROUGIER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Nicolas P. Rougier.
 * ============================================================================
 *
 * This source is based on the article by Jukka Jylänki :
 * "A Thousand Ways to Pack the Bin - A Practical Approach to
 * Two-Dimensional Rectangle Bin Packing", February 27, 2010.
 *
 * More precisely, this is an implementation of the Skyline Bottom-Left
 * algorithm based on C++ sources provided by Jukka Jylänki at:
 * http://clb.demon.fi/files/RectangleBinPack/
 *
 *  ============================================================================
 */
#pragma once


/**
 * @file   texture-atlas.h
 * @author Nicolas Rougier (Nicolas.Rougier@inria.fr)
 *
 * @defgroup texture-atlas Texture atlas
 *
 * A texture atlas is used to pack several small regions into a single texture.
 *
 * The actual implementation is based on the article by Jukka Jylänki : "A
 * Thousand Ways to Pack the Bin - A Practical Approach to Two-Dimensional
 * Rectangle Bin Packing", February 27, 2010.
 * More precisely, this is an implementation of the Skyline Bottom-Left
 * algorithm based on C++ sources provided by Jukka Jylänki at:
 * http://clb.demon.fi/files/RectangleBinPack/
 *
 *
 */

#include "common/memory.h"
#include "color.h"

namespace caspar { namespace core { namespace text {

struct rect
{
	int x;
	int y;
	int width;
	int height;
};

class texture_atlas
{
public:
	texture_atlas(const size_t w, const size_t h, const size_t d);

	rect get_region(int width, int height) const;
	void set_region(const size_t x, const size_t y, const size_t width, const size_t height, const unsigned char *data, const size_t stride, const color<double>& col);
	void clear();

	size_t depth() const;
	size_t width() const;
	size_t height() const;

	const uint8_t* data() const;

private:
	struct impl;
	caspar::spl::shared_ptr<impl> impl_;
};

}}}
