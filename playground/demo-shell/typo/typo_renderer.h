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

#ifndef MIR_TYPO_RENDERER_H_
#define MIR_TYPO_RENDERER_H_

namespace mir { namespace typo {

class Renderer
{
public:
    class Image
    {
    public:
        Image();
        Image(Image const&) = delete;
        Image(Image const&&) = delete;
        Image& operator=(Image const&) = delete;
        ~Image();

        typedef enum {alpha8} Format;

        void reserve(int w, int h, Format f);
        unsigned char* data() const { return buf; };
        int width() const { return width_; }
        int height() const { return height_; }
        int stride() const { return stride_; }
        int alignment() const { return align_; }
        Format format() const { return format_; }

    private:
        unsigned char* buf;
        int width_, stride_, height_, align_;
        Format format_;
    };

    virtual ~Renderer();
    virtual void render(char const* str, Image& img) = 0;

protected:
    static unsigned long unicode_from_utf8(char const** utf8);
};

} } // namespace mir::typo

#endif // MIR_TYPO_RENDERER_H_
