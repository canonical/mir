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

#ifndef MIR_EXAMPLES_TYPO_RENDERER_H_
#define MIR_EXAMPLES_TYPO_RENDERER_H_

namespace mir { namespace examples { namespace typo {

class Renderer
{
public:
    struct Image
    {
        Image();
        ~Image();
        typedef enum {alpha8} Format;
        void reserve(int w, int h, Format f);
        unsigned char* buf;
        int width, stride, height, align;
        Format format;
    };

    virtual ~Renderer();
    virtual void render(char const* str, Image& img) = 0;

protected:
    static unsigned long unicode_from_utf8(char const** utf8);
};

} } } // namespace mir::examples::typo

#endif // MIR_EXAMPLES_TYPO_RENDERER_H_
