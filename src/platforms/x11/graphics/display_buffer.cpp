/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/fatal.h"
#include "platform.h"
#include "display_buffer.h"
#include "display_configuration.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/transformation.h"
#include <cstring>

namespace mg=mir::graphics;
namespace mgx=mg::X;
namespace geom=mir::geometry;

mgx::DisplayBuffer::DisplayBuffer(std::shared_ptr<Platform> parent,
                                  xcb_window_t win,
                                  geometry::Rectangle const& view_area,
                                  geometry::Size pixel_size)
                                  : parent{std::move(parent)},
                                    area{view_area},
                                    in_pixels{pixel_size},
                                    transform(1),
                                    x_win{win}
{
}

geom::Rectangle mgx::DisplayBuffer::view_area() const
{
    return area;
}

auto mgx::DisplayBuffer::pixel_size() const -> geom::Size
{
    return in_pixels;
}

auto mgx::DisplayBuffer::overlay(std::vector<DisplayElement> const& /*renderlist*/) -> bool
{
    // We could, with a lot of effort, make something like overlay support work, but
    // there's little point.
    return false;
}

namespace
{
template<typename To, typename From>
auto unique_ptr_cast(std::unique_ptr<From> ptr) -> std::unique_ptr<To>
{
    From* unowned_src = ptr.release();
    if (auto to_src = dynamic_cast<To*>(unowned_src))
    {
        return std::unique_ptr<To>{to_src};
    }
    delete unowned_src;
    BOOST_THROW_EXCEPTION((
        std::bad_cast()));
}
}

void mgx::DisplayBuffer::set_next_image(std::unique_ptr<Framebuffer> content)
{
    next_frame = unique_ptr_cast<helpers::Framebuffer>(std::move(content));
}

glm::mat2 mgx::DisplayBuffer::transformation() const
{
    return transform;
}

void mgx::DisplayBuffer::set_view_area(geom::Rectangle const& a)
{
    area = a;
}

auto mgx::DisplayBuffer::display_provider() const -> std::shared_ptr<DisplayInterfaceProvider>
{
    return parent->provider_for_window(x_win);
}

void mgx::DisplayBuffer::set_transformation(glm::mat2 const& t)
{
    transform = t;
}

void mgx::DisplayBuffer::for_each_display_buffer(
    std::function<void(graphics::DisplayBuffer&)> const& f)
{
    f(*this);
}

void mgx::DisplayBuffer::post()
{
    next_frame->swap_buffers();
    next_frame.reset();
}

std::chrono::milliseconds mgx::DisplayBuffer::recommended_sleep() const
{
    return std::chrono::milliseconds::zero();
}

auto mgx::DisplayBuffer::x11_window() const -> xcb_window_t
{
    return x_win;
}