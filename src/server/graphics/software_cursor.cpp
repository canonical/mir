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
    size_t const pixels_size =
        cursor_image.size().width.as_uint32_t() *
        cursor_image.size().height.as_uint32_t() *
        MIR_BYTES_PER_PIXEL(format);

    if (pixels_size == 0)
        BOOST_THROW_EXCEPTION(std::logic_error("zero sized software cursor image is invalid"));

    auto new_renderable = std::make_shared<detail::CursorRenderable>(
        allocator->alloc_buffer({cursor_image.size(), format, mg::BufferUsage::software}),
        position + hotspot - cursor_image.hotspot());

    // TODO: The buffer pixel format may not be argb_8888, leading to
    // incorrect cursor colors. We need to transform the data to match
    // the buffer pixel format.
    auto pixel_source = dynamic_cast<mrs::PixelSource*>(new_renderable->buffer()->native_buffer_base());
    if (pixel_source)
        pixel_source->write(static_cast<unsigned char const*>(cursor_image.as_argb_8888()), pixels_size);
    else
        BOOST_THROW_EXCEPTION(std::logic_error("could not write to buffer for software cursor"));
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
