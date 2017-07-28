/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TOOLS_IMAGE_RENDERER_H_
#define MIR_TOOLS_IMAGE_RENDERER_H_

#include "mir/geometry/size.h"

#include <GLES2/gl2.h>
#include <cstdint>

namespace mir
{
namespace tools
{

class ImageRenderer
{
public:
    ImageRenderer(const uint8_t* pixel_data, mir::geometry::Size size,
                  uint32_t bytes_per_pixel);

    void render();

private:
    class Resources
    {
    public:
        ~Resources();
        void setup();

        GLuint vertex_shader;
        GLuint fragment_shader;
        GLuint program;
        GLuint position_attr_loc;
        GLuint vertex_attribs_vbo;
        GLuint texture;
    };

    Resources resources;
};

}
}

#endif /* MIR_TOOLS_IMAGE_RENDERER_H_ */
