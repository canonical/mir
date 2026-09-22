/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define MIR_LOG_COMPONENT "BlitterRenderer"

#include <mir/renderers/blitter/renderer.h>

#include "egl_context.h"
#include "framebuffer_pool.h"
#include "texture_cache.h"

#include "common/gl_output_filter.h"
#include "common/gl_program_factory.h"
#include "common/gl_scene_drawing.h"

#include <mir/graphics/renderable.h>
#include <mir/graphics/rendering_providers.h>
#include <mir/log.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mgl = mir::gl;
namespace mrb = mir::renderer::blitter;
namespace mrc = mir::renderer::common;

namespace
{
/**
 * Is a straight copy of this renderable's pixels the correct result?
 *
 * `blit()` takes neither an alpha nor a blend mode, so anything that needs
 * blending has to be drawn with GL instead. A buffer with an alpha channel is
 * still safe to copy where the renderable declares that region opaque.
 */
auto is_opaque(mg::Renderable const& renderable, geom::Rectangle const& target) -> bool
{
    if (renderable.alpha() != 1.0f)
        return false;

    if (!renderable.shaped())
        return true;

    auto const opaque = renderable.opaque_region();
    if (!opaque)
        return false;

    return std::ranges::any_of(*opaque, [&target](auto const& rect) { return rect.contains(target); });
}

/// Scale `fraction` of `source` into the corresponding part of `sample`
auto proportional_subrect(
    geom::RectangleD const& sample,
    geom::Rectangle const& source,
    geom::Rectangle const& fraction) -> geom::RectangleD
{
    if (source.size.width.as_int() <= 0 || source.size.height.as_int() <= 0)
        return sample;

    auto const scale_x = sample.size.width.as_value() / source.size.width.as_value();
    auto const scale_y = sample.size.height.as_value() / source.size.height.as_value();

    return geom::RectangleD{
        {sample.top_left.x.as_value() + (fraction.left() - source.left()).as_value() * scale_x,
         sample.top_left.y.as_value() + (fraction.top() - source.top()).as_value() * scale_y},
        {fraction.size.width.as_value() * scale_x,
         fraction.size.height.as_value() * scale_y}};
}

/**
 * Convert a fractional rectangle to a whole-pixel one, if it is one.
 *
 * `blit()` takes whole-pixel source rectangles, but a renderable may sample a
 * fractional region of its buffer. Rounding would sample the wrong pixels, so
 * anything fractional has to be drawn with GL instead.
 */
auto as_integral(geom::RectangleD const& rect) -> std::optional<geom::Rectangle>
{
    auto const values = {
        rect.top_left.x.as_value(), rect.top_left.y.as_value(),
        rect.size.width.as_value(), rect.size.height.as_value()};

    if (!std::ranges::all_of(values, [](auto v) { return v == std::floor(v); }))
        return std::nullopt;

    return geom::Rectangle{
        {static_cast<int>(rect.top_left.x.as_value()), static_cast<int>(rect.top_left.y.as_value())},
        {static_cast<int>(rect.size.width.as_value()), static_cast<int>(rect.size.height.as_value())}};
}

auto to_byte(float colour) -> uint8_t
{
    return static_cast<uint8_t>(std::clamp(colour, 0.0f, 1.0f) * 255.0f + 0.5f);
}
}

class mrb::Renderer::Impl
{
public:
    Impl(
        std::shared_ptr<mg::BlitterRenderingProvider> blitter,
        mg::CPUAddressableDisplayAllocator& allocator,
        std::shared_ptr<mg::GLConfig const> config)
        : blitter{std::move(blitter)},
          context{std::make_shared<SoftwareEGLContext>()},
          pool{FramebufferPool::create(context, this->blitter, allocator, *config)},
          textures{this->blitter},
          program_factory{std::make_unique<mrc::GLProgramFactory>()},
          filter{std::make_unique<mrc::GLOutputFilter>()}
    {
    }

    ~Impl()
    {
        /* We're about to release GL resources, so we need their context current.
         * Everything was allocated on our own context, which no one else uses.
         */
        try
        {
            context->make_current();
        }
        catch (...)
        {
            mir::log_warning("Failed to make EGL context current to release blitter renderer");
        }

        filter.reset();
        program_factory.reset();
        textures.invalidate();
        pool.reset();
    }

    void set_viewport(geom::Rectangle const& rect)
    {
        if (rect == viewport)
            return;

        viewport = rect;
        screen_to_gl_coords = mrc::screen_to_gl_coords_for(rect);
    }

    void set_output_transform(glm::mat2 const& t)
    {
        /* Our framebuffers are laid out top-row-first, but GL renders
         * bottom-row-first, so draw everything upside-down.
         */
        display_transform = glm::mat4{
            1.0, 0.0, 0.0, 0.0,
            0.0, -1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0
        } * glm::mat4(t);

        output_transform = t;
    }

    void set_output_filter(MirOutputFilter new_filter)
    {
        context->make_current();
        filter->set_filter(new_filter);
    }

    auto render(mg::RenderableList const& renderables) -> std::unique_ptr<mg::Framebuffer>;

    void suspend()
    {
        context->release_current();
    }

private:
    /**
     * Can the blitter be used for this frame at all?
     *
     * An output filter needs a whole-output GL pass, and anything but a
     * one-to-one mapping from scene coordinates to output pixels needs
     * scaling/rotation GL does for free. In either case, blitting a renderable
     * and then correcting it with GL would be strictly more work than just
     * drawing the frame with GL.
     */
    auto can_blit_frame() const -> bool
    {
        return !filter->active()
            && output_transform == glm::mat2{1}
            && viewport.size == scene.output_size;
    }

    /// Where in the target framebuffer `area` of the scene lands
    auto to_target(geom::Rectangle const& area) const -> geom::Rectangle
    {
        return {area.top_left - as_displacement(viewport.top_left), area.size};
    }

    void draw_with_gl(mg::Renderable const& renderable);

    /**
     * Hand a single renderable to the blitter
     *
     * \param [in] target_rect  Where in the target the renderable should land,
     *                          already clipped
     * \param [in] begin_task   Ensures a Task is active; returns false if one
     *                          could not be started
     * \return  false if this renderable must be drawn with GL instead
     */
    auto try_blit(
        mg::Renderable const& renderable,
        geom::Rectangle const& target_rect,
        auto const& begin_task) -> bool
    {
        if (!target_rect.size.width.as_int() || !target_rect.size.height.as_int())
            return true;    // Entirely clipped away; nothing to draw

        if (!is_opaque(renderable, target_rect) || renderable.transformation() != glm::mat4{1})
            return false;

        auto const source_rect = as_integral(
            proportional_subrect(
                renderable.src_bounds(), to_target(renderable.screen_position()), target_rect));
        if (!source_rect)
            return false;

        return begin_task() &&
            blitter->blit(
                *task,
                *renderable.buffer(),
                *source_rect,
                target_rect,
                renderable.orientation(),
                renderable.mirror_mode());
    }
    std::shared_ptr<mg::BlitterRenderingProvider> const blitter;
    std::shared_ptr<SoftwareEGLContext> const context;
    std::shared_ptr<FramebufferPool> pool;
    TextureCache textures;
    std::unique_ptr<mrc::GLProgramFactory> program_factory;
    std::unique_ptr<mrc::GLOutputFilter> filter;

    GLfloat clear_colour[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    geom::Rectangle viewport;
    glm::mat2 output_transform{1};
    glm::mat4 display_transform{1};
    glm::mat4 screen_to_gl_coords{0};
    long long frameno{0};
    std::vector<mgl::Primitive> primitives;

    /* Valid only for the duration of render() */
    mrc::SceneContext scene{};
    FramebufferPool::Entry* target{nullptr};
    std::unique_ptr<mg::BlitterRenderingProvider::Task> task;
};

auto mrb::Renderer::Impl::render(mg::RenderableList const& renderables) -> std::unique_ptr<mg::Framebuffer>
{
    auto checkout = pool->acquire();
    target = &checkout.entry;

    scene = mrc::SceneContext{
        .viewport = viewport,
        .output_size = target->size(),
        .display_transform = display_transform,
        .screen_to_gl_coords = screen_to_gl_coords,
        .frameno = ++frameno};

    auto const use_blitter = can_blit_frame();

    /* Any GL drawing has to wait for the blitter to finish with the target, and
     * the blitter then has to wait for GL, so each transition costs a full
     * pipeline flush in both directions.
     */
    auto const end_task =
        [this]
        {
            if (task)
                target->surface = blitter->wait_complete(std::move(task));
        };
    auto const begin_task =
        [this]
        {
            if (!task && target->surface)
                task = blitter->create_task(std::move(target->surface));
            return static_cast<bool>(task);
        };

    if (use_blitter && begin_task())
    {
        if (!blitter->fill(
                *task, to_target(viewport),
                to_byte(clear_colour[0]), to_byte(clear_colour[1]),
                to_byte(clear_colour[2]), to_byte(clear_colour[3])))
        {
            end_task();
            target->bind_gl();
            mrc::clear(clear_colour);
            glFinish();
        }

        for (auto const& renderable : renderables)
        {
            auto const target_rect =
                [&]
                {
                    auto const position = to_target(renderable->screen_position());
                    if (auto const clip = renderable->clip_area())
                        return intersection_of(position, to_target(*clip));
                    return position;
                }();

            if (!try_blit(*renderable, target_rect, begin_task))
            {
                end_task();
                draw_with_gl(*renderable);
                begin_task();
            }
        }

        end_task();
    }
    else
    {
        target->bind_gl();
        if (filter->active())
            filter->bind_intermediate(scene.output_size);

        mrc::set_letterboxed_gl_viewport(viewport, display_transform, scene.output_size);
        mrc::clear(clear_colour);

        for (auto const& renderable : renderables)
            draw_with_gl(*renderable);

        if (filter->active())
        {
            target->bind_gl();
            mrc::set_letterboxed_gl_viewport(viewport, display_transform, scene.output_size);
            filter->apply();
        }

        glFinish();
    }

    while (auto const gl_error = glGetError())
        mir::log_debug("GL error: %d", gl_error);

    textures.drop_unused();
    target = nullptr;

    return std::move(checkout.framebuffer);
}

/**
 * Draw a single renderable into the target with GL.
 *
 * The caller must have ended any outstanding blitter Task first; this blocks
 * until the draw has landed in the target so that the blitter may resume.
 */
void mrb::Renderer::Impl::draw_with_gl(mg::Renderable const& renderable)
try
{
    target->bind_gl();
    mrc::set_letterboxed_gl_viewport(viewport, display_transform, scene.output_size);

    auto& texture = textures.get(*renderable.buffer());

    primitives.clear();
    mrc::tessellate(primitives, renderable);

    mrc::draw(scene, renderable, texture, *program_factory, primitives);

    /* The blitter has no way to wait on GL, so the draw has to have landed in
     * the target before we hand the target back to it.
     */
    glFinish();
}
catch (std::exception const& err)
{
    mir::log_warning("Failed to draw a renderable: %s", err.what());
}

mrb::Renderer::Renderer(
    std::shared_ptr<mg::BlitterRenderingProvider> blitter,
    mg::CPUAddressableDisplayAllocator& allocator,
    std::shared_ptr<mg::GLConfig const> config)
    : impl{std::make_unique<Impl>(std::move(blitter), allocator, std::move(config))}
{
}

mrb::Renderer::~Renderer() = default;

void mrb::Renderer::set_viewport(geom::Rectangle const& rect)
{
    impl->set_viewport(rect);
}

void mrb::Renderer::set_output_transform(glm::mat2 const& t)
{
    impl->set_output_transform(t);
}

void mrb::Renderer::set_output_filter(MirOutputFilter filter)
{
    impl->set_output_filter(filter);
}

auto mrb::Renderer::render(mg::RenderableList const& renderables) const -> std::unique_ptr<mg::Framebuffer>
{
    return impl->render(renderables);
}

void mrb::Renderer::suspend()
{
    impl->suspend();
}
