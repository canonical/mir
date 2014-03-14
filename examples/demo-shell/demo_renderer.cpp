/*
 * Copyright © 2014 Canonical Ltd.
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

#include "demo_renderer.h"
#include <mir/graphics/renderable.h>
#include <cmath>

using namespace mir;
using namespace mir::examples;

namespace
{

GLuint generate_shadow_texture(float opacity)
{
    GLuint shadow_tex = 0;
    const int width = 256;
    const int height = 1;
    GLubyte shadow_tex_image[width * 2];
    GLubyte *px = shadow_tex_image;
    for (int x = 0; x < width; ++x)
    {
         px[0] = 0;
         px[1] = opacity * 255.0f * (1.0f - sinf(x * M_PI / (width * 2)));
         px += 2;
    }
    glGenTextures(1, &shadow_tex);
    glBindTexture(GL_TEXTURE_2D, shadow_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                 width, height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                 shadow_tex_image);
    return shadow_tex;
}

} // namespace

DemoRenderer::DemoRenderer(geometry::Rectangle const& display_area)
    : GLRenderer(display_area)
{
    shadow_tex = generate_shadow_texture(0.4f);
}

void DemoRenderer::begin() const
{
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void DemoRenderer::tessellate(graphics::Renderable const& renderable,
                              std::vector<Primitive>& primitives) const
{
    GLRenderer::tessellate(renderable, primitives);

    auto const& rect = renderable.screen_position();
    GLfloat left = rect.top_left.x.as_int();
    GLfloat right = left + rect.size.width.as_int();
    GLfloat top = rect.top_left.y.as_int();
    GLfloat bottom = top + rect.size.height.as_int();

    float radius = 80.0f; // TODO configurable?

    auto n = primitives.size();
    primitives.resize(n + 4);

    auto& right_shadow = primitives[n];
    right_shadow.tex_id = shadow_tex;
    right_shadow.type = GL_TRIANGLE_FAN;
    right_shadow.vertices.resize(4);
    right_shadow.vertices[0] = {{right,          top,    0.0f}, {0.0f, 0.0f}};
    right_shadow.vertices[1] = {{right + radius, top,    0.0f}, {1.0f, 0.0f}};
    right_shadow.vertices[2] = {{right + radius, bottom, 0.0f}, {1.0f, 1.0f}};
    right_shadow.vertices[3] = {{right,          bottom, 0.0f}, {0.0f, 1.0f}};

    auto& left_shadow = primitives[n+1];
    left_shadow.tex_id = shadow_tex;
    left_shadow.type = GL_TRIANGLE_FAN;
    left_shadow.vertices.resize(4);
    left_shadow.vertices[0] = {{left - radius, top,    0.0f}, {1.0f, 1.0f}};
    left_shadow.vertices[1] = {{left,          top,    0.0f}, {0.0f, 1.0f}};
    left_shadow.vertices[2] = {{left,          bottom, 0.0f}, {0.0f, 0.0f}};
    left_shadow.vertices[3] = {{left - radius, bottom, 0.0f}, {1.0f, 0.0f}};

    auto& top_shadow = primitives[n+2];
    top_shadow.tex_id = shadow_tex;
    top_shadow.type = GL_TRIANGLE_FAN;
    top_shadow.vertices.resize(4);
    top_shadow.vertices[0] = {{left,  top,          0.0f}, {0.0f, 0.0f}};
    top_shadow.vertices[1] = {{left,  top - radius, 0.0f}, {1.0f, 0.0f}};
    top_shadow.vertices[2] = {{right, top - radius, 0.0f}, {1.0f, 1.0f}};
    top_shadow.vertices[3] = {{right, top,          0.0f}, {0.0f, 1.0f}};

    auto& bottom_shadow = primitives[n+3];
    bottom_shadow.tex_id = shadow_tex;
    bottom_shadow.type = GL_TRIANGLE_FAN;
    bottom_shadow.vertices.resize(4);
    bottom_shadow.vertices[0] = {{left,  bottom + radius, 0.0f}, {1.0f, 1.0f}};
    bottom_shadow.vertices[1] = {{left,  bottom,          0.0f}, {0.0f, 1.0f}};
    bottom_shadow.vertices[2] = {{right, bottom,          0.0f}, {0.0f, 0.0f}};
    bottom_shadow.vertices[3] = {{right, bottom + radius, 0.0f}, {1.0f, 0.0f}};

    // Shadows always need blending...
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
