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

#ifndef MIR_EXAMPLES_DEMO_RENDERER_H_
#define MIR_EXAMPLES_DEMO_RENDERER_H_

#include "mir/compositor/gl_renderer.h"
#include "mir/compositor/decoration.h"
#include "typo_glcache.h"

#include <unordered_map>

namespace mir
{
namespace examples
{

enum ColourEffect
{
    none,
    inverse,
    contrast,
    neffects
};

typedef std::unordered_map<graphics::Renderable::ID,
                           std::unique_ptr<compositor::Decoration>> DecorMap;

class DemoRenderer : public compositor::GLRenderer
{
public:
    DemoRenderer(
        geometry::Rectangle const& display_area,
        float const titlebar_height,
        float const shadow_radius);
    ~DemoRenderer();

    void begin(DecorMap&&) const;
    void set_colour_effect(ColourEffect);

protected:
    void tessellate(std::vector<graphics::GLPrimitive>& primitives,
                    graphics::Renderable const& renderable) const override;

    void draw(graphics::Renderable const& renderable,
              GLRenderer::Program const& prog) const override;

private:
    void tessellate_shadow(
        std::vector<graphics::GLPrimitive>& primitives,
        graphics::Renderable const& renderable,
        float radius) const;
    void tessellate_frame(
        std::vector<graphics::GLPrimitive>& primitives,
        graphics::Renderable const& renderable,
        float titlebar_height,
        char const* name) const;

    float const titlebar_height;
    float const shadow_radius;
    float const corner_radius;
    GLuint shadow_corner_tex;
    GLuint titlebar_corner_tex;

    ColourEffect colour_effect;
    Program inverse_program, contrast_program;
    
    mutable DecorMap decor_map;
    mutable typo::GLCache title_cache;
};

} // namespace examples
} // namespace mir

#endif // MIR_EXAMPLES_DEMO_RENDERER_H_
