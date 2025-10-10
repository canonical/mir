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

#include "software_cursor.h"
#include "mir/graphics/cursor_image.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/pixel_format_utils.h"
#include "mir/graphics/renderable.h"
#include "mir/input/scene.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/executor.h"
#include "mir/graphics/pixman_image_scaling.h"

#include <boost/throw_exception.hpp>

#include <memory>
#include <stdexcept>
#include <mutex>

namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mrs = mir::renderer::software;

namespace
{

MirPixelFormat get_8888_format(std::vector<MirPixelFormat> const& formats)
{
    for (auto format : formats)
    {
        if (mg::red_channel_depth(format) == 8 &&
            mg::green_channel_depth(format) == 8 &&
            mg::blue_channel_depth(format) == 8 &&
            mg::alpha_channel_depth(format) == 8)
        {
            return format;
        }
    }

    return mir_pixel_format_invalid;
}

}

class mg::detail::CursorRenderable : public mg::Renderable
{
public:
    CursorRenderable(std::shared_ptr<mg::Buffer> const& buffer,
                     geom::Point const& position)
        : buffer_{buffer},
          position{position}
    {
    }

    mg::Renderable::ID id() const override
    {
        return this;
    }

    std::shared_ptr<mg::Buffer> buffer() const override
    {
        return buffer_;
    }

    geom::Rectangle screen_position() const override
    {
        std::lock_guard lock{position_mutex};
        return {position, buffer_->size() * scale};
    }

    geom::RectangleD src_bounds() const override
    {
        return {{0, 0}, buffer_->size()};
    }

    std::optional<geometry::Rectangle> clip_area() const override
    {
        return std::optional<geometry::Rectangle>();
    }

    float alpha() const override
    {
        return 1.0;
    }

    glm::mat4 transformation() const override
    {
        return glm::mat4(1);
    }

    MirOrientation orientation() const override
    {
        return mir_orientation_normal;
    }

    MirMirrorMode mirror_mode() const override
    {
        return mir_mirror_mode_none;
    }

    bool shaped() const override
    {
        return true;
    }

    auto surface_if_any() const
        -> std::optional<mir::scene::Surface const*> override
    {
        return std::nullopt;
    }

    void move_to(geom::Point new_position)
    {
        std::lock_guard lock{position_mutex};
        position = new_position;
    }

    void set_scale(float new_scale)
    {
        scale = new_scale;
    }

    auto opaque_region() const -> std::optional<geom::Rectangles> override
    {
        return geom::Rectangles{{screen_position()}};
    }

private:
    std::shared_ptr<mg::Buffer> const buffer_;
    mutable std::mutex position_mutex;
    geom::Point position;
    float scale = 1.f;
};

mg::SoftwareCursor::SoftwareCursor(
    std::shared_ptr<GraphicBufferAllocator> const& allocator,
    std::shared_ptr<input::Scene> const& scene)
    : allocator{allocator},
      scene{scene},
      format{get_8888_format(allocator->supported_pixel_formats())},
      visible(false),
      hotspot{0,0}
{
}

mg::SoftwareCursor::~SoftwareCursor() = default;

void mg::SoftwareCursor::show(std::shared_ptr<CursorImage> const& cursor_image)
{
    {
        std::lock_guard lg{guard};

        // Store the cursor image for later use with `set_scale`
        current_cursor_image = cursor_image;

        geom::Point position{0, 0};
        if (renderable_)
            position = renderable_->screen_position().top_left;

        renderable_ = create_scaled_renderable_for(*current_cursor_image, position);

        hotspot = current_cursor_image->hotspot() * current_scale;
        visible = true;
    }
    scene->emit_scene_changed();
}

std::shared_ptr<mg::detail::CursorRenderable>
mg::SoftwareCursor::create_scaled_renderable_for(CursorImage const& cursor_image, geom::Point position)
{
    if (cursor_image.size().width.as_uint32_t() == 0 || cursor_image.size().height.as_uint32_t() == 0)
        BOOST_THROW_EXCEPTION(std::logic_error("zero sized software cursor image is invalid"));

    auto buffer = mrs::alloc_buffer_with_content(
        *allocator,
        static_cast<unsigned char const*>(cursor_image.as_argb_8888()),
        cursor_image.size(),
        geom::Stride{
            cursor_image.size().width.as_uint32_t() * MIR_BYTES_PER_PIXEL(mir_pixel_format_argb_8888)},
        mir_pixel_format_argb_8888);

    auto new_renderable = std::make_shared<detail::CursorRenderable>(
        std::move(buffer),
        position + hotspot - cursor_image.hotspot() * current_scale);

    new_renderable->set_scale(current_scale);

    return new_renderable;
}

void mg::SoftwareCursor::hide()
{
    {
        std::lock_guard lg{guard};
        if (!visible)
            return;

        visible = false;
    }
    scene->emit_scene_changed();
}

void mg::SoftwareCursor::move_to(geometry::Point position)
{
    {
        std::lock_guard lg{guard};
        if (!renderable_)
            return;

        renderable_->move_to(position - hotspot);
    }

    // This doesn't need to be called in a specific order with other potential calls, so it doesn't go on the executor
    scene->emit_scene_changed();
}

void mir::graphics::SoftwareCursor::scale(float new_scale)
{
    {
        std::lock_guard lg{guard};
        current_scale = new_scale;
    }

    show(current_cursor_image);
}

auto mg::SoftwareCursor::renderable() -> std::shared_ptr<Renderable>
{
    std::lock_guard lg{guard};
    if (!visible)
        return nullptr;

    return renderable_;
}

auto mg::SoftwareCursor::needs_compositing() const -> bool
{
    std::lock_guard lg{guard};
    return visible;
}
