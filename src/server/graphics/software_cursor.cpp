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

#include <pixman-1/pixman.h>
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
        return {position, buffer_->size()};
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

private:
    std::shared_ptr<mg::Buffer> const buffer_;
    mutable std::mutex position_mutex;
    geom::Point position;
};

mg::SoftwareCursor::SoftwareCursor(
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<Executor> const& scene_executor,
    std::shared_ptr<mi::Scene> const& scene)
    : allocator{allocator}, scene{scene},
      format{get_8888_format(allocator->supported_pixel_formats())},
      scene_executor{scene_executor},
      visible(false),
      hotspot{0,0}
{
}

mg::SoftwareCursor::~SoftwareCursor()
{
    hide();
}

void mg::SoftwareCursor::show(CursorImage const& cursor_image)
{
    std::lock_guard lg{guard};

    // Store the cursor image for later use with `set_scale`
    this->current_cursor_image = &cursor_image;

    // TODO Make this user configurable
    set_scale(1.75);

    visible = true;
}

std::shared_ptr<mg::detail::CursorRenderable>
mg::SoftwareCursor::create_renderable_for(CursorImage const& cursor_image, geom::Point position)
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
        position + hotspot - cursor_image.hotspot());

    return new_renderable;
}

void mg::SoftwareCursor::hide()
{
    std::lock_guard lg{guard};

    if (visible && renderable)
    {
        scene_executor->spawn([scene = scene, to_remove = renderable]()
            {
                scene->remove_input_visualization(to_remove);
            });
    }

    visible = false;
}

void mg::SoftwareCursor::move_to(geometry::Point position)
{
    {
        std::lock_guard lg{guard};

        if (!renderable)
            return;

        renderable->move_to(position - hotspot);
    }

    // This doesn't need to be called in a specific order with other potential calls, so it doesn't go on the executor
    scene->emit_scene_changed();
}

void mir::graphics::SoftwareCursor::set_scale(float new_scale)
{
    auto const [width, height] = current_cursor_image->size();

    auto* original_image = pixman_image_create_bits(
        PIXMAN_a8r8g8b8,
        width.as_int(),
        height.as_int(),
        (uint32_t*)(current_cursor_image->as_argb_8888()),
        width.as_int() * MIR_BYTES_PER_PIXEL(mir_pixel_format_argb_8888));

    pixman_transform_t transform;
    pixman_transform_init_identity(&transform);
    // Counter-intuitively, you use the INVERSE transform, not the "normal" one
    pixman_transform_scale(NULL, &transform, pixman_double_to_fixed(new_scale), pixman_double_to_fixed(new_scale));
    pixman_image_set_transform(original_image, &transform);
    pixman_image_set_filter(original_image, PIXMAN_FILTER_BILINEAR, NULL, 0);

    auto const scaled_width = width.as_value() * new_scale;
    auto const scaled_height = height.as_value() * new_scale;
    auto buf = std::make_unique<uint32_t[]>(scaled_width * scaled_height);
    auto* scaled_image = pixman_image_create_bits(
        PIXMAN_a8r8g8b8,
        scaled_width,
        scaled_height,
        buf.get(),
        scaled_width * MIR_BYTES_PER_PIXEL(mir_pixel_format_argb_8888));

    // Copy over the pixels into the larger image
    pixman_image_composite(
        PIXMAN_OP_SRC, original_image, NULL, scaled_image, 0, 0, 0, 0, 0, 0, scaled_width, scaled_height);

    auto buffer = mrs::alloc_buffer_with_content(
        *allocator,
        reinterpret_cast<unsigned char*>(pixman_image_get_data(scaled_image)),
        {scaled_width, scaled_height},
        geom::Stride{scaled_width * MIR_BYTES_PER_PIXEL(mir_pixel_format_argb_8888)},
        mir_pixel_format_argb_8888);

    auto const to_remove = visible ? renderable : nullptr;
    auto const position = renderable ? renderable->screen_position().top_left : geom::Point{0, 0};

    renderable = std::make_shared<detail::CursorRenderable>(
        std::move(buffer), position + hotspot - current_cursor_image->hotspot());

    hotspot = current_cursor_image->hotspot() * new_scale;

    scene_executor->spawn(
        [scene = scene, to_remove = to_remove, to_add = renderable]()
        {
            // Add the new renderable first, then remove the old one to avoid visual glitches
            scene->add_input_visualization(to_add);

            if (to_remove)
            {
                scene->remove_input_visualization(to_remove);
            }
        });

    pixman_image_unref(original_image);
    pixman_image_unref(scaled_image);
}
