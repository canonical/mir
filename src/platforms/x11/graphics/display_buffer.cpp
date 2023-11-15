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
#include "../x11_resources.h"
#include <cstring>

namespace mg=mir::graphics;
namespace mgx=mg::X;
namespace geom=mir::geometry;

class mgx::DisplaySink::Allocator : public mg::GenericEGLDisplayAllocator
{
public:
    Allocator(std::shared_ptr<helpers::EGLHelper> egl, xcb_connection_t* connection, xcb_window_t window)
        : egl{std::move(egl)},
          x11_connection{connection},
          x11_win{window}
    {
    }

    auto alloc_framebuffer(mg::GLConfig const& config, EGLContext share_context)
    -> std::unique_ptr<EGLFramebuffer> override
    {
        return egl->framebuffer_for_window(config, x11_connection, x11_win, share_context);
    }
private:
    std::shared_ptr<helpers::EGLHelper> const egl;
    xcb_connection_t* const x11_connection;
    xcb_window_t const x11_win;
};

mgx::DisplaySink::DisplaySink(
    xcb_connection_t* connection,
    xcb_window_t win,
    std::shared_ptr<helpers::EGLHelper> egl,
    geometry::Rectangle const& view_area,
    geometry::Size pixel_size)
    : area{view_area},
      in_pixels{pixel_size},
      transform(1),
      egl{std::move(egl)},
      x11_connection{connection},
      x_win{win}
{
}

mgx::DisplaySink::~DisplaySink() = default;

geom::Rectangle mgx::DisplaySink::view_area() const
{
    return area;
}

auto mgx::DisplaySink::pixel_size() const -> geom::Size
{
    return in_pixels;
}

auto mgx::DisplaySink::overlay(std::vector<DisplayElement> const& /*renderlist*/) -> bool
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

void mgx::DisplaySink::set_next_image(std::unique_ptr<Framebuffer> content)
{
    next_frame = unique_ptr_cast<helpers::Framebuffer>(std::move(content));
}

glm::mat2 mgx::DisplaySink::transformation() const
{
    return transform;
}

void mgx::DisplaySink::set_view_area(geom::Rectangle const& a)
{
    area = a;
}

void mgx::DisplaySink::set_transformation(glm::mat2 const& t)
{
    transform = t;
}

void mgx::DisplaySink::for_each_display_sink(std::function<void(graphics::DisplaySink & )> const& f)
{
    f(*this);
}

void mgx::DisplaySink::post()
{
    next_frame->swap_buffers();
    next_frame.reset();
}

std::chrono::milliseconds mgx::DisplaySink::recommended_sleep() const
{
    return std::chrono::milliseconds::zero();
}

auto mgx::DisplaySink::x11_window() const -> xcb_window_t
{
    return x_win;
}

auto mgx::DisplaySink::maybe_create_allocator(mg::DisplayAllocator::Tag const& type_tag)
    -> DisplayAllocator*
{
    if (dynamic_cast<GenericEGLDisplayAllocator::Tag const*>(&type_tag))
    {
        if (!egl_allocator)
        {
            egl_allocator = std::make_unique<Allocator>(egl, x11_connection, x_win);
        }
        return egl_allocator.get();
    }
    return nullptr;
}
