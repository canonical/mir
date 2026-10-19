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

#include "framebuffer_pool.h"
#include "software_egl_context.h"
#include "texture_cache.h"

#include "common/gl_output_filter.h"
#include "common/gl_program_factory.h"
#include "common/gl_scene_drawing.h"

#include <mir/graphics/buffer.h>
#include <mir/graphics/renderable.h>
#include <mir/graphics/rendering_providers.h>
#include <mir/log.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <tuple>
#include <utility>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mgl = mir::gl;
namespace mrb = mir::renderer::blitter;
namespace mrc = mir::renderer::common;

namespace
{
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

auto to_byte(float colour) -> uint8_t
{
    return static_cast<uint8_t>(std::lround(std::clamp(colour, 0.0f, 1.0f) * 255.0f));
}

/// The part of the scene `renderable` covers within `viewport`, after clipping
auto visible_area_of(mg::Renderable const& renderable, geom::Rectangle const& viewport) -> geom::Rectangle
{
    auto const on_output = intersection_of(renderable.screen_position(), viewport);
    if (auto const clip = renderable.clip_area())
        return intersection_of(on_output, *clip);
    return on_output;
}
}

class mrb::Renderer::Impl
{
public:
    Impl(
        std::shared_ptr<mg::BlitterRenderingProvider> blitter,
        mg::CPUAddressableDisplayAllocator& allocator)
        : blitter{std::move(blitter)},
          context{std::make_shared<SoftwareEGLContext>()},
          pool{context, this->blitter, allocator},
          textures{context, this->blitter}
    {
        context->make_current();
        program_factory = std::make_unique<mrc::GLProgramFactory>();
        filter = std::make_unique<mrc::GLOutputFilter>();
    }

    ~Impl()
    {
        auto const restore_egl_state = preserve_egl_state_unless_current(*context);
        try
        {
            context->make_current();
        }
        catch (...)
        {
            mir::log_warning("Failed to make EGL context current to release blitter renderer; leaking GL objects");
            try
            {
                context->release_current();
            }
            catch (...)
            {
                std::ignore = filter.release();
                std::ignore = program_factory.release();
            }
        }

        filter.reset();
        program_factory.reset();
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

    auto render(mg::RenderableList const& renderables) -> mg::FramebufferPtr;

    void suspend()
    {
        if (context->is_current())
            context->release_current();
    }

private:
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

    void blit_frame(mg::RenderableList const& renderables);
    void draw_frame_with_gl(mg::RenderableList const& renderables);

    /// Ensure a blitter [Task] is active on the target
    ///
    /// \return false if one could not be started
    auto begin_task() -> bool
    {
        if (!task)
        {
            if (auto surface = target->take_surface())
                task = blitter->create_task(std::move(surface));
        }
        return static_cast<bool>(task);
    }

    /// Wait for any active blitter [Task], returning the target's surface to it
    void end_task()
    {
        if (task)
            target->return_surface(blitter->wait_complete(std::move(task)));
    }

    void bind_target_for_gl()
    {
        used_gl = true;
        target->bind();
        mrc::set_letterboxed_gl_viewport(viewport, display_transform, scene.output_size);
    }

    /// Take the target back from the blitter, `draw` into it with GL, and wait for that to land
    void fall_back_to_gl(auto const& draw)
    {
        end_task();
        bind_target_for_gl();
        draw();
        glFinish();
    }

    auto fill_background() -> bool
    {
        return blitter->fill(
            *task, to_target(viewport),
            to_byte(clear_colour[0]), to_byte(clear_colour[1]),
            to_byte(clear_colour[2]), to_byte(clear_colour[3]));
    }

    static void log_gl_errors()
    {
        while (auto const gl_error = glGetError())
            mir::log_debug("GL error: %d", gl_error);
    }

    /// Hand a single renderable to the blitter
    ///
    /// \param visible_area The part of the scene the renderable covers, already clipped
    /// \return  `false` if this renderable must be drawn with GL instead
    auto try_blit(mg::Renderable const& renderable, geom::Rectangle const& visible_area) -> bool
    {
        if (!visible_area.size.width.as_int() || !visible_area.size.height.as_int())
            return true;    // Entirely clipped away.

        if (renderable.transformation() != glm::mat4{1} || !is_opaque(renderable, visible_area))
            return false;

        auto const position = renderable.screen_position();
        auto const is_clipped = visible_area != position;
        auto const is_transformed =
            renderable.orientation() != mir_orientation_normal || renderable.mirror_mode() != mir_mirror_mode_none;

        // TODO: work out which part of the buffer a clipped area samples if it is rotated or flipped
        if (is_clipped && is_transformed)
            return false;

        // `blit()` takes a whole-pixel source, so truncate (all values are non-negative, so this floors).
        // Being up to a pixel off is an acceptable price for staying on the blitter.
        auto const source = proportional_subrect(renderable.src_bounds(), position, visible_area);
        auto const source_rect = geom::Rectangle{geom::Point{source.top_left}, geom::Size{source.size}};
        if (!source_rect.size.width.as_int() || !source_rect.size.height.as_int())
            return false;   // Samples less than a pixel of the buffer, and only GL can stretch that

        return begin_task() &&
            blitter->blit(
                *task,
                *renderable.buffer(),
                source_rect,
                to_target(visible_area),
                renderable.orientation(),
                renderable.mirror_mode());
    }

    std::shared_ptr<mg::BlitterRenderingProvider> const blitter;
    std::shared_ptr<SoftwareEGLContext> const context;
    FramebufferPool pool;
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
    std::optional<FramebufferPool::PooledFB> target;
    std::unique_ptr<mg::BlitterRenderingProvider::Task> task;
    bool used_gl{false};
};

auto mrb::Renderer::Impl::render(mg::RenderableList const& renderables) -> mg::FramebufferPtr
{
    target.emplace(pool.acquire());
    used_gl = false;

    scene = mrc::SceneContext{
        .viewport = viewport,
        .output_size = target->size(),
        .display_transform = display_transform,
        .screen_to_gl_coords = screen_to_gl_coords,
        .frameno = ++frameno};

    try
    {
        if (can_blit_frame() && begin_task())
            blit_frame(renderables);
        else
            draw_frame_with_gl(renderables);
    }
    catch (...)
    {
        // Don't strand the target's surface in an abandoned Task
        end_task();
        target.reset();
        throw;
    }

    if (used_gl)
        log_gl_errors();

    textures.drop_unused();

    auto framebuffer = target->claim_fb();
    target.reset();
    return framebuffer;
}

void mrb::Renderer::Impl::blit_frame(mg::RenderableList const& renderables)
{
    if (!fill_background())
        fall_back_to_gl([this] { mrc::clear(clear_colour); });

    for (auto const& renderable : renderables)
    {
        if (!try_blit(*renderable, visible_area_of(*renderable, viewport)))
            fall_back_to_gl([&] { draw_with_gl(*renderable); });
    }

    end_task();
}

void mrb::Renderer::Impl::draw_frame_with_gl(mg::RenderableList const& renderables)
{
    bind_target_for_gl();
    auto const filtering = filter->bind_intermediate(scene.output_size);

    mrc::clear(clear_colour);

    for (auto const& renderable : renderables)
        draw_with_gl(*renderable);

    if (filtering)
    {
        target->bind();
        filter->apply();
    }

    glFinish();
}

void mrb::Renderer::Impl::draw_with_gl(mg::Renderable const& renderable)
{
    try
    {
        auto& texture = textures.get(*renderable.buffer());

        primitives.clear();
        mrc::tessellate(primitives, renderable);

        mrc::draw(scene, renderable, texture, *program_factory, primitives);
    }
    catch (std::exception const& err)
    {
        // Realistically, there's not much we can do here except log. If we get to
        // this point, something has gone terribly wrong, but we should at least
        // not crash the compositor or anything.
        mir::log_error("Failed to draw a renderable: %s", err.what());
    }
}

mrb::Renderer::Renderer(
    std::shared_ptr<mg::BlitterRenderingProvider> blitter,
    mg::CPUAddressableDisplayAllocator& allocator)
    : impl{std::make_unique<Impl>(std::move(blitter), allocator)}
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

auto mrb::Renderer::render(mg::RenderableList const& renderables) const -> mg::FramebufferPtr
{
    return impl->render(renderables);
}

void mrb::Renderer::suspend()
{
    impl->suspend();
}
