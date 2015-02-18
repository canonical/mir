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

#ifndef MIR_EXAMPLES_STUB_GL_TEXT_CACHE_
#define MIR_EXAMPLES_STUB_GL_TEXT_CACHE_

#include "gl_text_cache.h"

namespace mir { namespace examples {

class StubGLTextCache : public GLTextCache
{
protected:
    void render(char const* str, Image& img) override;
};

} } // namespace mir::examples

#endif // MIR_EXAMPLES_STUB_GL_TEXT_CACHE_
