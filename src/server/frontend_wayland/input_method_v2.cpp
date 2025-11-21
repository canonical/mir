/*
 * Copyright © Canonical Ltd.
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
 */

#include "input_method_v2.h"
#include "input_method_grab_keyboard_v2.h"
#include "text-input-unstable-v3_wrapper.h"
#include "wl_seat.h"
#include "input_method_common.h"

#include <mir/scene/text_input_hub.h>
#include <mir/log.h>

#include <deque>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace ms = mir::scene;

namespace mir
{
namespace frontend
{
struct InputMethodV2Ctx
{
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<scene::TextInputHub> const text_input_hub;
    std::shared_ptr<input::CompositeEventFilter> const event_filter;
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
    InputMethodV2(wl_resource* resource, WlSeat* seat, std::shared_ptr<InputMethodV2Ctx> const& ctx);
    ~InputMethodV2();

private:
    struct StateObserver : ms::TextInputStateObserver
    {
        StateObserver(InputMethodV2* input_method)
            : input_method{input_method}
        {
        }

        void activated(
            scene::TextInputStateSerial serial,
            bool new_input_field,
            scene::TextInputState const& state) override
        {
            input_method->activated(serial, new_input_field, state);
        }

        void deactivated() override
        {
            input_method->deactivated();
        }

        void show_input_panel() override {}
        void hide_input_panel() override {}

        InputMethodV2* const input_method;
    };

    static size_t constexpr max_remembered_serials{10};

    WlSeat* const seat;
    std::shared_ptr<InputMethodV2Ctx> const ctx;
    std::shared_ptr<StateObserver> const state_observer;
    bool is_activated{false};
    wayland::Weak<wayland::InputMethodKeyboardGrabV2> current_grab_keyboard;
    scene::TextInputState cached_state{};
    scene::TextInputChange pending_change{{}};
    /// The first value is the number of the done event that corresponds to the second value. When we get a commit it
    /// will have a done event count, which we need to connect back to the serial.
    std::deque<std::pair<uint32_t, ms::TextInputStateSerial>> serials;
    uint32_t done_event_count{0};

    /// Called by the state observer
    /// @{
    void activated(scene::TextInputStateSerial serial, bool new_input_field, scene::TextInputState const& state);
    void deactivated();
    /// @}

    /// Returns the serial that corresponds to the given done event count
    auto find_serial(uint32_t done_event_count) const -> std::optional<ms::TextInputStateSerial>;

    /// Wraps wayland::InputMethodV2::send_done_event() and increments done_event_count
    void send_done_event();

    /// Wayland requests
    /// @{
    void commit_string(std::string const& text) override;
    void set_preedit_string(std::string const& text, int32_t cursor_begin, int32_t cursor_end) override;
    void delete_surrounding_text(uint32_t before_length, uint32_t after_length) override;
    void commit(uint32_t serial) override;
    void get_input_popup_surface(struct wl_resource* id, struct wl_resource* surface) override;
    void grab_keyboard(struct wl_resource* keyboard) override;
    /// @}
};

// TODO: correctly implement popup surface
class InputPopupSurfaceV2
    : public wayland::InputPopupSurfaceV2
{
public:
    InputPopupSurfaceV2(wl_resource* resource);
};
}
}

auto mf::create_input_method_manager_v2(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<scene::TextInputHub> const& text_input_hub,
    std::shared_ptr<input::CompositeEventFilter> const event_filter)
-> std::shared_ptr<mw::InputMethodManagerV2::Global>
{
    auto ctx = std::shared_ptr<InputMethodV2Ctx>{new InputMethodV2Ctx{wayland_executor, text_input_hub, event_filter}};
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
    auto const wl_seat = WlSeat::from(seat);
    new InputMethodV2{input_method, wl_seat, ctx};
}

// InputMethodV2

mf::InputMethodV2::InputMethodV2(
    wl_resource* resource,
    WlSeat* seat,
    std::shared_ptr<InputMethodV2Ctx> const& ctx)
    : mw::InputMethodV2{resource, Version<1>()},
      seat{seat},
      ctx{ctx},
      state_observer{std::make_shared<StateObserver>(this)}
{
    ctx->text_input_hub->register_interest(state_observer, *ctx->wayland_executor);
}

mf::InputMethodV2::~InputMethodV2()
{
    ctx->text_input_hub->unregister_interest(*state_observer);
}

void mf::InputMethodV2::activated(
    ms::TextInputStateSerial serial,
    bool new_input_field,
    ms::TextInputState const& state)
{
    if (new_input_field || !is_activated)
    {
        if (is_activated)
        {
            send_deactivate_event();
            send_done_event();
        }
        is_activated = true;
        cached_state = ms::TextInputState{};
        send_activate_event();
    }
    if (cached_state.surrounding_text != state.surrounding_text ||
        cached_state.cursor != state.cursor ||
        cached_state.anchor != state.anchor)
    {
        cached_state.surrounding_text = state.surrounding_text;
        cached_state.cursor = state.cursor;
        cached_state.anchor = state.anchor;
        send_surrounding_text_event(
            state.surrounding_text.value_or(""),
            state.cursor.value_or(0),
            state.anchor.value_or(0));
    }
    if (cached_state.change_cause != state.change_cause)
    {
        cached_state.change_cause = state.change_cause;
        send_text_change_cause_event(mir_to_wayland_change_cause(
            state.change_cause.value_or(ms::TextInputChangeCause::other)));
    }
    if (cached_state.content_hint != state.content_hint || cached_state.content_purpose != state.content_purpose)
    {
        cached_state.content_hint = state.content_hint;
        cached_state.content_purpose = state.content_purpose;
        send_content_type_event(
            mir_to_wayland_content_hint(state.content_hint.value_or(ms::TextInputContentHint::none)),
            mir_to_wayland_content_purpose(state.content_purpose.value_or(ms::TextInputContentPurpose::normal)));
    }
    send_done_event();
    serials.push_back({done_event_count, serial});
    while (serials.size() > max_remembered_serials)
    {
        serials.pop_front();
    }
}

void mf::InputMethodV2::deactivated()
{
    if (is_activated)
    {
        is_activated = false;
        send_deactivate_event();
        send_done_event();
    }
}

auto mf::InputMethodV2::find_serial(uint32_t done_event_count) const -> std::optional<ms::TextInputStateSerial>
{
    // Loop in reverse order because the serial we're looking for will generally be at the end
    for (auto it = serials.rbegin(); it != serials.rend(); it++)
    {
        if (it->first == done_event_count)
        {
            return it->second;
        }
    }
    return std::nullopt;
}

void mf::InputMethodV2::send_done_event()
{
    done_event_count++;
    mw::InputMethodV2::send_done_event();
}

void mf::InputMethodV2::commit_string(std::string const& text)
{
    pending_change.commit_text = text;
}

void mf::InputMethodV2::set_preedit_string(std::string const& text, int32_t cursor_begin, int32_t cursor_end)
{
    pending_change.preedit_text = text;
    pending_change.preedit_cursor_begin = cursor_begin;
    pending_change.preedit_cursor_end = cursor_end;
}

void mf::InputMethodV2::delete_surrounding_text(uint32_t before_length, uint32_t after_length)
{
    pending_change.delete_before = before_length;
    pending_change.delete_after = after_length;
}

void mf::InputMethodV2::commit(uint32_t serial)
{
    auto const mir_serial = find_serial(serial);
    if (mir_serial)
    {
        pending_change.serial = *mir_serial;
        ctx->text_input_hub->text_changed(pending_change);
    }
    else
    {
        log_warning("%s: invalid commit serial %d", interface_name, serial);
    }
    pending_change = ms::TextInputChange{{}};
}

void mf::InputMethodV2::get_input_popup_surface(struct wl_resource* id, struct wl_resource* surface)
{
    (void)surface;
    new InputPopupSurfaceV2{id};
}

void mf::InputMethodV2::grab_keyboard(struct wl_resource* keyboard)
{
    if (!is_activated)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            std::string{} + "got " + interface_name + ".grab_keyboard when input method was not active"));
    }
    if (current_grab_keyboard)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("tried to grab the keyboard multiple times"));
    }
    current_grab_keyboard = mw::make_weak<wayland::InputMethodKeyboardGrabV2>(
        new InputMethodGrabKeyboardV2{keyboard, *seat, ctx->wayland_executor, *ctx->event_filter});
}

// InputPopupSurfaceV2

mf::InputPopupSurfaceV2::InputPopupSurfaceV2(wl_resource* resource)
    : mw::InputPopupSurfaceV2{resource, Version<1>()}
{
}
