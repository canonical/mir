/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_EXAMPLES_TYPO_FREETYPE_RENDERER_H_
#define MIR_EXAMPLES_TYPO_FREETYPE_RENDERER_H_

#include "typo_renderer.h"
#include <ft2build.h>
#include FT_FREETYPE_H

namespace mir { namespace examples { namespace typo {

class FreetypeRenderer : public Renderer
{
public:
    FreetypeRenderer();
    ~FreetypeRenderer();
    bool load(char const* font_path, int pref_height);
    void render(char const* str, Image& img) override;

private:
    FT_Library lib;
    FT_Face face;
    int preferred_height;
};

} } } // namespace mir::examples::typo

#endif // MIR_EXAMPLES_TYPO_FREETYPE_RENDERER_H_
