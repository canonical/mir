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

#include "input_method_v2.h"
#include "input-method-unstable-v2_wrapper.h"

namespace mf = mir::frontend;
namespace mw = mir::wayland;

namespace mir
{
namespace frontend
{
struct InputMethodV2Ctx
{
};

class InputMethodManagerV2Global
    : public wayland::InputMethodManagerV2::Global
{
public:
    InputMethodManagerV2Global(wl_display* display, std::shared_ptr<InputMethodV2Ctx> const& ctx);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<InputMethodV2Ctx> const ctx;
};

class InputMethodManagerV2
    : public wayland::InputMethodManagerV2
{
public:
    InputMethodManagerV2(wl_resource* resource, std::shared_ptr<InputMethodV2Ctx> const& ctx);

private:
    void get_input_method(struct wl_resource* seat, struct wl_resource* input_method) override;

    std::shared_ptr<InputMethodV2Ctx> const ctx;
};

class InputMethodV2
    : public wayland::InputMethodV2
{
public:
    InputMethodV2(wl_resource* resource, std::shared_ptr<InputMethodV2Ctx> const& ctx);

private:
    std::shared_ptr<InputMethodV2Ctx> const ctx;

    void commit_string(std::string const& text) override;
    void preedit_string(std::string const& text, int32_t cursor_begin, int32_t cursor_end) override;
    void delete_surrounding_text(uint32_t before_length, uint32_t after_length) override;
    void commit(uint32_t serial) override;
    void get_input_popup_surface(struct wl_resource* id, struct wl_resource* surface) override;
    void grab_keyboard(struct wl_resource* keyboard) override;
};

class InputPopupSurfaceV2
    : public wayland::InputPopupSurfaceV2
{
public:
    InputPopupSurfaceV2(wl_resource* resource, InputMethodV2* input_method);

private:
    wayland::Weak<InputMethodV2> input_method;
};
}
}

auto mf::create_input_method_manager_v2(wl_display* display) -> std::shared_ptr<InputMethodManagerV2Global>
{
    auto ctx = std::shared_ptr<InputMethodV2Ctx>{new InputMethodV2Ctx{}};
    return std::make_shared<InputMethodManagerV2Global>(display, std::move(ctx));
}

// InputMethodManagerV2Global

mf::InputMethodManagerV2Global::InputMethodManagerV2Global(
    wl_display* display,
    std::shared_ptr<InputMethodV2Ctx> const& ctx)
    : Global{display, Version<1>()},
      ctx{ctx}
{
}

void mf::InputMethodManagerV2Global::bind(wl_resource* new_resource)
{
    new InputMethodManagerV2{new_resource, ctx};
}

// InputMethodManagerV2

mf::InputMethodManagerV2::InputMethodManagerV2(
    wl_resource* resource,
    std::shared_ptr<InputMethodV2Ctx> const& ctx)
    : mw::InputMethodManagerV2{resource, Version<1>()},
      ctx{ctx}
{
}

void mf::InputMethodManagerV2::get_input_method(struct wl_resource* seat, struct wl_resource* input_method)
{
    (void)seat;
    new InputMethodV2{input_method, ctx};
}

// InputMethodV2

mf::InputMethodV2::InputMethodV2(
    wl_resource* resource,
    std::shared_ptr<InputMethodV2Ctx> const& ctx)
    : mw::InputMethodV2{resource, Version<1>()},
      ctx{ctx}
{
}

void mf::InputMethodV2::commit_string(std::string const& text)
{
    (void)text;
}

void mf::InputMethodV2::preedit_string(std::string const& text, int32_t cursor_begin, int32_t cursor_end)
{
    (void)text;
    (void)cursor_begin;
    (void)cursor_end;
}

void mf::InputMethodV2::delete_surrounding_text(uint32_t before_length, uint32_t after_length)
{
    (void)before_length;
    (void)after_length;
}

void mf::InputMethodV2::commit(uint32_t serial)
{
    (void)serial;
}

void mf::InputMethodV2::get_input_popup_surface(struct wl_resource* id, struct wl_resource* surface)
{
    (void)surface;
    new InputPopupSurfaceV2{id, this};
}

void mf::InputMethodV2::grab_keyboard(struct wl_resource* keyboard)
{
    (void)keyboard;
}

// InputPopupSurfaceV2

mf::InputPopupSurfaceV2::InputPopupSurfaceV2(wl_resource* resource, InputMethodV2* input_method)
    : mw::InputPopupSurfaceV2{resource, Version<1>()},
      input_method{input_method}
{
}
