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

float penumbra_curve(float x)
{
    return 1.0f - std::sin(x * M_PI / 2.0f);
}

GLuint generate_shadow_corner_texture(float opacity)
{
    struct Texel
    {
        GLubyte luminance;
        GLubyte alpha;
    };

    int const width = 256;
    Texel image[width][width];

    int const max = width - 1;
    for (int y = 0; y < width; ++y)
    {
        float curve_y = opacity * 255.0f *
                        penumbra_curve(static_cast<float>(y) / max);
        for (int x = 0; x < width; ++x)
        {
            Texel *t = &image[y][x];
            t->luminance = 0;
            t->alpha = curve_y * penumbra_curve(static_cast<float>(x) / max);
        }
    }

    GLuint corner;
    glGenTextures(1, &corner);
    glBindTexture(GL_TEXTURE_2D, corner);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                 width, width, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                 image);

    return corner;
}

GLuint generate_frame_corner_texture()
{
    struct Texel
    {
        GLubyte r, g, b, a;
    };

    int const width = 256;
    Texel image[width][width];

    int cx = width / 2;
    int cy = width / 2;
    int radius_sqr = cx * cx;

    for (int y = 0; y < width; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            GLubyte lum = 128;
            GLubyte alpha = 255;

            // Cut out the corner in a circular shape.
            if (x < cx && y < cy)
            {
                int dx = cx - x;
                int dy = cy - y;
                if (dx * dx + dy * dy >= radius_sqr)
                    alpha = 0;
            }

            // Set gradient
            if (y < cy)
            {
                float brighten = (1.0f - (static_cast<float>(y) / cy));
                if (x < cx)
                    brighten *= std::sin(x * M_PI / width);

                lum += (255 - lum) * brighten;
            }

            image[y][x] = {lum, lum, lum, alpha};
        }
    }

    GLuint corner;
    glGenTextures(1, &corner);
    glBindTexture(GL_TEXTURE_2D, corner);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                   GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 width, width, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image);
    glGenerateMipmap(GL_TEXTURE_2D); // Antialiasing please

    return corner;
}

} // namespace

DemoRenderer::DemoRenderer(geometry::Rectangle const& display_area)
    : GLRenderer(display_area)
{
    shadow_corner_tex = generate_shadow_corner_texture(0.4f);
    titlebar_corner_tex = generate_frame_corner_texture();
}

DemoRenderer::~DemoRenderer()
{
    glDeleteTextures(1, &shadow_corner_tex);
    glDeleteTextures(1, &titlebar_corner_tex);
}

void DemoRenderer::begin() const
{
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void DemoRenderer::tessellate(std::vector<Primitive>& primitives,
                              graphics::Renderable const& renderable,
                              geometry::Size const& buf_size) const
{
    GLRenderer::tessellate(primitives, renderable, buf_size);
    tessellate_shadow(primitives, renderable, 80.0f);
    tessellate_frame(primitives, renderable, 30.0f);
}

void DemoRenderer::tessellate_shadow(std::vector<Primitive>& primitives,
                                     graphics::Renderable const& renderable,
                                     float radius) const
{
    auto const& rect = renderable.screen_position();
    GLfloat left = rect.top_left.x.as_int();
    GLfloat right = left + rect.size.width.as_int();
    GLfloat top = rect.top_left.y.as_int();
    GLfloat bottom = top + rect.size.height.as_int();

    auto n = primitives.size();
    primitives.resize(n + 8);

    GLfloat rightr = right + radius;
    GLfloat leftr = left - radius;
    GLfloat topr = top - radius;
    GLfloat bottomr = bottom + radius;

    auto& right_shadow = primitives[n++];
    right_shadow.tex_id = shadow_corner_tex;
    right_shadow.type = GL_TRIANGLE_FAN;
    right_shadow.vertices.resize(4);
    right_shadow.vertices[0] = {{right,  top,    0.0f}, {0.0f, 0.0f}};
    right_shadow.vertices[1] = {{rightr, top,    0.0f}, {1.0f, 0.0f}};
    right_shadow.vertices[2] = {{rightr, bottom, 0.0f}, {1.0f, 0.0f}};
    right_shadow.vertices[3] = {{right,  bottom, 0.0f}, {0.0f, 0.0f}};

    auto& left_shadow = primitives[n++];
    left_shadow.tex_id = shadow_corner_tex;
    left_shadow.type = GL_TRIANGLE_FAN;
    left_shadow.vertices.resize(4);
    left_shadow.vertices[0] = {{leftr, top,    0.0f}, {1.0f, 0.0f}};
    left_shadow.vertices[1] = {{left,  top,    0.0f}, {0.0f, 0.0f}};
    left_shadow.vertices[2] = {{left,  bottom, 0.0f}, {0.0f, 0.0f}};
    left_shadow.vertices[3] = {{leftr, bottom, 0.0f}, {1.0f, 0.0f}};

    auto& top_shadow = primitives[n++];
    top_shadow.tex_id = shadow_corner_tex;
    top_shadow.type = GL_TRIANGLE_FAN;
    top_shadow.vertices.resize(4);
    top_shadow.vertices[0] = {{left,  topr, 0.0f}, {1.0f, 0.0f}};
    top_shadow.vertices[1] = {{right, topr, 0.0f}, {1.0f, 0.0f}};
    top_shadow.vertices[2] = {{right, top,  0.0f}, {0.0f, 0.0f}};
    top_shadow.vertices[3] = {{left,  top,  0.0f}, {0.0f, 0.0f}};

    auto& bottom_shadow = primitives[n++];
    bottom_shadow.tex_id = shadow_corner_tex;
    bottom_shadow.type = GL_TRIANGLE_FAN;
    bottom_shadow.vertices.resize(4);
    bottom_shadow.vertices[0] = {{left,  bottom,  0.0f}, {0.0f, 0.0f}};
    bottom_shadow.vertices[1] = {{right, bottom,  0.0f}, {0.0f, 0.0f}};
    bottom_shadow.vertices[2] = {{right, bottomr, 0.0f}, {1.0f, 0.0f}};
    bottom_shadow.vertices[3] = {{left,  bottomr, 0.0f}, {1.0f, 0.0f}};

    auto& tr_shadow = primitives[n++];
    tr_shadow.tex_id = shadow_corner_tex;
    tr_shadow.type = GL_TRIANGLE_FAN;
    tr_shadow.vertices.resize(4);
    tr_shadow.vertices[0] = {{right,  top,  0.0f}, {0.0f, 0.0f}};
    tr_shadow.vertices[1] = {{right,  topr, 0.0f}, {1.0f, 0.0f}};
    tr_shadow.vertices[2] = {{rightr, topr, 0.0f}, {1.0f, 1.0f}};
    tr_shadow.vertices[3] = {{rightr, top,  0.0f}, {0.0f, 1.0f}};

    auto& br_shadow = primitives[n++];
    br_shadow.tex_id = shadow_corner_tex;
    br_shadow.type = GL_TRIANGLE_FAN;
    br_shadow.vertices.resize(4);
    br_shadow.vertices[0] = {{right,  bottom,  0.0f}, {0.0f, 0.0f}};
    br_shadow.vertices[1] = {{rightr, bottom,  0.0f}, {1.0f, 0.0f}};
    br_shadow.vertices[2] = {{rightr, bottomr, 0.0f}, {1.0f, 1.0f}};
    br_shadow.vertices[3] = {{right,  bottomr, 0.0f}, {0.0f, 1.0f}};

    auto& bl_shadow = primitives[n++];
    bl_shadow.tex_id = shadow_corner_tex;
    bl_shadow.type = GL_TRIANGLE_FAN;
    bl_shadow.vertices.resize(4);
    bl_shadow.vertices[0] = {{left,  bottom,  0.0f}, {0.0f, 0.0f}};
    bl_shadow.vertices[1] = {{left,  bottomr, 0.0f}, {1.0f, 0.0f}};
    bl_shadow.vertices[2] = {{leftr, bottomr, 0.0f}, {1.0f, 1.0f}};
    bl_shadow.vertices[3] = {{leftr, bottom,  0.0f}, {0.0f, 1.0f}};

    auto& tl_shadow = primitives[n++];
    tl_shadow.tex_id = shadow_corner_tex;
    tl_shadow.type = GL_TRIANGLE_FAN;
    tl_shadow.vertices.resize(4);
    tl_shadow.vertices[0] = {{left,  top,  0.0f}, {0.0f, 0.0f}};
    tl_shadow.vertices[1] = {{leftr, top,  0.0f}, {1.0f, 0.0f}};
    tl_shadow.vertices[2] = {{leftr, topr, 0.0f}, {1.0f, 1.0f}};
    tl_shadow.vertices[3] = {{left,  topr, 0.0f}, {0.0f, 1.0f}};

    // Shadows always need blending...
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void DemoRenderer::tessellate_frame(std::vector<Primitive>& primitives,
                                    graphics::Renderable const& renderable,
                                    float titlebar_height) const
{
    auto const& rect = renderable.screen_position();
    GLfloat left = rect.top_left.x.as_int();
    GLfloat right = left + rect.size.width.as_int();
    GLfloat top = rect.top_left.y.as_int();

    auto n = primitives.size();
    primitives.resize(n + 3);

    GLfloat htop = top - titlebar_height;
    GLfloat inleft = left + titlebar_height;  // Square proportions for corners
    GLfloat inright = right - titlebar_height;

    GLfloat mid = (left + right) / 2.0f;
    if (inleft > mid) inleft = mid;
    if (inright < mid) inright = mid;

    auto& top_left_corner = primitives[n++];
    top_left_corner.tex_id = titlebar_corner_tex;
    top_left_corner.type = GL_TRIANGLE_FAN;
    top_left_corner.vertices.resize(4);
    top_left_corner.vertices[0] = {{left,   htop, 0.0f}, {0.0f, 0.0f}};
    top_left_corner.vertices[1] = {{inleft, htop, 0.0f}, {1.0f, 0.0f}};
    top_left_corner.vertices[2] = {{inleft, top,  0.0f}, {1.0f, 1.0f}};
    top_left_corner.vertices[3] = {{left,   top,  0.0f}, {0.0f, 1.0f}};

    auto& top_right_corner = primitives[n++];
    top_right_corner.tex_id = titlebar_corner_tex;
    top_right_corner.type = GL_TRIANGLE_FAN;
    top_right_corner.vertices.resize(4);
    top_right_corner.vertices[0] = {{inright, htop, 0.0f}, {1.0f, 0.0f}};
    top_right_corner.vertices[1] = {{right,   htop, 0.0f}, {0.0f, 0.0f}};
    top_right_corner.vertices[2] = {{right,   top,  0.0f}, {0.0f, 1.0f}};
    top_right_corner.vertices[3] = {{inright, top,  0.0f}, {1.0f, 1.0f}};

    auto& titlebar = primitives[n++];
    titlebar.tex_id = titlebar_corner_tex;
    titlebar.type = GL_TRIANGLE_FAN;
    titlebar.vertices.resize(4);
    titlebar.vertices[0] = {{inleft,  htop, 0.0f}, {1.0f, 0.0f}};
    titlebar.vertices[1] = {{inright, htop, 0.0f}, {1.0f, 0.0f}};
    titlebar.vertices[2] = {{inright, top,  0.0f}, {1.0f, 1.0f}};
    titlebar.vertices[3] = {{inleft,  top,  0.0f}, {1.0f, 1.0f}};
}

