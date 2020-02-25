/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "software_cursor.h"
#include "mir/graphics/cursor_image.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/pixel_format_utils.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/input/scene.h"
#include "mir/renderer/sw/pixel_source.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <mutex>
#include <cstring>

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

    unsigned int swap_interval() const override
    {
        return 1;
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
        std::lock_guard<std::mutex> lock{position_mutex};
        return {position, buffer_->size()};
    }

    std::experimental::optional<geometry::Rectangle> clip_area() const override
    {
        return std::experimental::optional<geometry::Rectangle>();
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

    void move_to(geom::Point new_position)
    {
        std::lock_guard<std::mutex> lock{position_mutex};
        position = new_position;
    }

private:
    std::shared_ptr<mg::Buffer> const buffer_;
    mutable std::mutex position_mutex;
    geom::Point position;
};

mg::SoftwareCursor::SoftwareCursor(
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<mi::Scene> const& scene)
    : allocator{allocator},
      scene{scene},
      format{get_8888_format(allocator->supported_pixel_formats())},
      visible(false),
      hotspot{0,0}
{
}

mg::SoftwareCursor::~SoftwareCursor()
{
    hide();
}

void mg::SoftwareCursor::show()
{
    bool needs_scene_change = false;
    {
        std::lock_guard<std::mutex> lg{guard};
        if (!visible)
            visible = needs_scene_change = true;
    }
    if (needs_scene_change && renderable)
        scene->add_input_visualization(renderable);
}

void mg::SoftwareCursor::show(CursorImage const& cursor_image)
{
    std::shared_ptr<detail::CursorRenderable> new_renderable;
    std::shared_ptr<detail::CursorRenderable> old_renderable;
    bool old_visibility = false;
    // Do a lock dance to make this function threadsafe,
    // while avoiding calling scene methods under lock
    {
        geom::Point position{0,0};
        std::lock_guard<std::mutex> lg{guard};
        if (renderable)
            position = renderable->screen_position().top_left;
        new_renderable = create_renderable_for(cursor_image, position);
        old_visibility = visible;
        visible = true;
    }

    // Add the new renderable first, then remove the old one to avoid
    // visual glitches
    scene->add_input_visualization(new_renderable);

    // The second part of the lock dance
    {
        std::lock_guard<std::mutex> lg{guard};
        old_renderable = renderable;
        renderable = new_renderable;
        hotspot = cursor_image.hotspot();
    }

    if (old_renderable && old_visibility)
        scene->remove_input_visualization(old_renderable);
}

std::shared_ptr<mg::detail::CursorRenderable>
mg::SoftwareCursor::create_renderable_for(CursorImage const& cursor_image, geom::Point position)
{
    if (cursor_image.size().width.as_uint32_t() == 0 || cursor_image.size().height.as_uint32_t() == 0)
        BOOST_THROW_EXCEPTION(std::logic_error("zero sized software cursor image is invalid"));

    auto new_renderable = std::make_shared<detail::CursorRenderable>(
        allocator->alloc_software_buffer(cursor_image.size(), format),
        position + hotspot - cursor_image.hotspot());

    auto const mapping = mrs::as_write_mappable_buffer(new_renderable->buffer())->map_writeable();

    if (mapping->format() == mir_pixel_format_argb_8888)
    {
        if (
            mapping->stride().as_uint32_t() ==
            MIR_BYTES_PER_PIXEL(mir_pixel_format_argb_8888) * cursor_image.size().width.as_uint32_t())
        {
            // Happy case: Buffer is packed, like the cursor_image; we can just blit.
            ::memcpy(
                mapping->data(),
                static_cast<unsigned char const*>(cursor_image.as_argb_8888()),
                mapping->len());
        }
        else
        {
            // Less happy path: the buffer has a different stride; we need to copy row-by-row
            auto const dest_stride = mapping->stride().as_uint32_t();
            auto const src_stride = cursor_image.size().width.as_uint32_t() * MIR_BYTES_PER_PIXEL(mir_pixel_format_argb_8888);
            for (auto y = 0u; y < cursor_image.size().height.as_uint32_t(); ++y)
            {
                ::memcpy(
                    mapping->data() + (dest_stride * y),
                    static_cast<unsigned char const*>(cursor_image.as_argb_8888()) + src_stride * y,
                    src_stride);
            }
        }
    }
    else if (mapping->format() == mir_pixel_format_abgr_8888)
    {
        // We need to do format conversion. Yay!
        // On the plus side, this means we don't have to do anything special to handle stride.
        auto const dest_stride = mapping->stride().as_uint32_t();
        auto const src_stride = cursor_image.size().width.as_uint32_t() * MIR_BYTES_PER_PIXEL(format);
        for (auto y = 0u; y < cursor_image.size().height.as_uint32_t(); ++y)
        {
            for (auto x = 0u; x < cursor_image.size().width.as_uint32_t(); ++x)
            {
                auto* const dst_pixel =
                    reinterpret_cast<uint32_t*>(
                        mapping->data() + (dest_stride * y) + (x * sizeof(uint32_t)));
                auto* const src_pixel =
                    reinterpret_cast<uint32_t const*>(
                        static_cast<unsigned char const*>(cursor_image.as_argb_8888()) +
                        (src_stride * y) +
                        (sizeof(uint32_t) * x));
                // TODO: This should be trivially-SIMD-able, for performance.
                auto const a = (*src_pixel >> 24) & 255;
                auto const r = (*src_pixel >> 16) & 255;
                auto const g = (*src_pixel >> 8) & 255;
                auto const b = (*src_pixel >> 0) & 255;

                *dst_pixel =
                    (a << 24) |
                    (b << 16) |
                    (g << 8)  |
                    (r << 0);
            }
        }
    }
    else
    {
        // There should be only two 8888 formats, and we should have covered them above!
        BOOST_THROW_EXCEPTION((std::logic_error{"Unexpected buffer format for cursor"}));
    }
    return new_renderable;
}

void mg::SoftwareCursor::hide()
{
    bool needs_scene_change = false;
    {
        std::lock_guard<std::mutex> lg{guard};
        if (visible)
        {
            visible = false;
            needs_scene_change = true;
        }
    }
    if (needs_scene_change && renderable)
        scene->remove_input_visualization(renderable);
}

void mg::SoftwareCursor::move_to(geometry::Point position)
{
    {
        std::lock_guard<std::mutex> lg{guard};

        if (!renderable)
            return;

        renderable->move_to(position - hotspot);
    }

    scene->emit_scene_changed();
}
