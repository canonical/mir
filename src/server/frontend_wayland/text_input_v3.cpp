/*
 * Copyright © 2021 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "text_input_v3.h"
#include "text-input-unstable-v3_wrapper.h"

#include "wl_seat.h"
#include "wl_surface.h"

namespace mf = mir::frontend;
namespace mw = mir::wayland;

namespace mir
{
namespace frontend
{
struct TextInputV3Ctx
{
};

class TextInputManagerV3Global
    : public wayland::TextInputManagerV3::Global
{
public:
    TextInputManagerV3Global(wl_display* display, std::shared_ptr<TextInputV3Ctx> const& ctx);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<TextInputV3Ctx> const ctx;
};

class TextInputManagerV3
    : public wayland::TextInputManagerV3
{
public:
    TextInputManagerV3(wl_resource* resource, std::shared_ptr<TextInputV3Ctx> const& ctx);

private:
    void get_text_input(struct wl_resource* id, struct wl_resource* seat) override;

    std::shared_ptr<TextInputV3Ctx> const ctx;
};

class TextInputV3
    : public wayland::TextInputV3,
      private WlSeat::FocusListener
{
public:
    TextInputV3(wl_resource* resource, std::shared_ptr<TextInputV3Ctx> const& ctx, WlSeat& seat);
    ~TextInputV3();

private:
    std::shared_ptr<TextInputV3Ctx> const ctx;
    WlSeat& seat;
    wayland::Weak<WlSurface> current_surface;

    /// From WlSeat::FocusListener
    void focus_on(WlSurface* surface) override;

    /// From wayland::TextInputV3
    /// @{
    void enable() override;
    void disable() override;
    void set_surrounding_text(std::string const& text, int32_t cursor, int32_t anchor) override;
    void set_text_change_cause(uint32_t cause) override;
    void set_content_type(uint32_t hint, uint32_t purpose) override;
    void set_cursor_rectangle(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void commit() override;
    /// @}
};
}
}

auto mf::create_text_input_manager_v3(wl_display* display) -> std::shared_ptr<TextInputManagerV3Global>
{
    auto ctx = std::shared_ptr<TextInputV3Ctx>{new TextInputV3Ctx{}};
    return std::make_shared<TextInputManagerV3Global>(display, std::move(ctx));
}

mf::TextInputManagerV3Global::TextInputManagerV3Global(
    wl_display* display,
    std::shared_ptr<TextInputV3Ctx> const& ctx)
    : Global{display, Version<1>()},
      ctx{ctx}
{
}

void mf::TextInputManagerV3Global::bind(wl_resource* new_resource)
{
    new TextInputManagerV3{new_resource, ctx};
}

mf::TextInputManagerV3::TextInputManagerV3(
    wl_resource* resource,
    std::shared_ptr<TextInputV3Ctx> const& ctx)
    : wayland::TextInputManagerV3{resource, Version<1>()},
      ctx{ctx}
{
}

void mf::TextInputManagerV3::get_text_input(struct wl_resource* id, struct wl_resource* seat)
{
    auto const wl_seat = WlSeat::from(seat);
    if (!wl_seat)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("failed to resolve WlSeat creating TextInputV3"));
    }
    new TextInputV3{id, ctx, *wl_seat};
}

mf::TextInputV3::TextInputV3(
    wl_resource* resource,
    std::shared_ptr<TextInputV3Ctx> const& ctx,
    WlSeat& seat)
    : wayland::TextInputV3{resource, Version<1>()},
      ctx{ctx},
      seat{seat}
{
    seat.add_focus_listener(client, this);
}

mf::TextInputV3::~TextInputV3()
{
    seat.remove_focus_listener(client, this);
}

void mf::TextInputV3::focus_on(WlSurface* surface)
{
    if (current_surface)
    {
        send_leave_event(current_surface.value().resource);
    }
    current_surface = mw::make_weak(surface);
    if (surface)
    {
        send_enter_event(surface->resource);
    }
}

void mf::TextInputV3::enable()
{
}

void mf::TextInputV3::disable()
{
}

void mf::TextInputV3::set_surrounding_text(std::string const& text, int32_t cursor, int32_t anchor)
{
    (void)text;
    (void)cursor;
    (void)anchor;
}

void mf::TextInputV3::set_text_change_cause(uint32_t cause)
{
    (void)cause;
}

void mf::TextInputV3::set_content_type(uint32_t hint, uint32_t purpose)
{
    (void)hint;
    (void)purpose;
}

void mf::TextInputV3::set_cursor_rectangle(int32_t x, int32_t y, int32_t width, int32_t height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void mf::TextInputV3::commit()
{
}
