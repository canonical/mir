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
#include "input_method_common.h"
#include "wl_seat.h"

#include <mir/log.h>
#include <mir/scene/text_input_hub.h>

#include <boost/throw_exception.hpp>
#include <deque>
#include <ranges>

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;
namespace ms = mir::scene;

namespace
{
struct InputMethodV2Ctx
{
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<ms::TextInputHub> const text_input_hub;
    std::shared_ptr<mir::input::CompositeEventFilter> const event_filter;
};

class InputPopupSurfaceV2
    : public mw::InputPopupSurfaceV2
{
public:
    InputPopupSurfaceV2(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::InputPopupSurfaceV2Middleware> instance,
        uint32_t object_id)
        : mw::InputPopupSurfaceV2{std::move(client), std::move(instance), object_id}
    {
    }
};

class InputMethodV2
    : public mw::InputMethodV2
{
public:
    InputMethodV2(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::InputMethodV2Middleware> instance,
        uint32_t object_id,
        mf::WlSeat* seat,
        std::shared_ptr<InputMethodV2Ctx> const& ctx)
        : mw::InputMethodV2{std::move(client), std::move(instance), object_id},
          seat{seat},
          ctx{ctx},
          state_observer{std::make_shared<StateObserver>(this)}
    {
        ctx->text_input_hub->register_interest(state_observer, *ctx->wayland_executor);
    }

    ~InputMethodV2() override
    {
        ctx->text_input_hub->unregister_interest(*state_observer);
    }

private:
    struct StateObserver : ms::TextInputStateObserver
    {
        explicit StateObserver(InputMethodV2* input_method)
            : input_method{input_method}
        {
        }

        void activated(
            ms::TextInputStateSerial serial,
            bool new_input_field,
            ms::TextInputState const& state) override
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

    mf::WlSeat* const seat;
    std::shared_ptr<InputMethodV2Ctx> const ctx;
    std::shared_ptr<StateObserver> const state_observer;
    bool is_activated{false};
    mw::Weak<mw::InputMethodKeyboardGrabV2> current_grab_keyboard;
    ms::TextInputState cached_state{};
    ms::TextInputChange pending_change{{}};
    std::deque<std::pair<uint32_t, ms::TextInputStateSerial>> serials;
    uint32_t done_event_count{0};

    void activated(ms::TextInputStateSerial serial, bool new_input_field, ms::TextInputState const& state)
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
            send_text_change_cause_event(mf::mir_to_wayland_change_cause(
                state.change_cause.value_or(ms::TextInputChangeCause::other)));
        }
        if (cached_state.content_hint != state.content_hint || cached_state.content_purpose != state.content_purpose)
        {
            cached_state.content_hint = state.content_hint;
            cached_state.content_purpose = state.content_purpose;
            send_content_type_event(
                mf::mir_to_wayland_content_hint(state.content_hint.value_or(ms::TextInputContentHint::none)),
                mf::mir_to_wayland_content_purpose(state.content_purpose.value_or(ms::TextInputContentPurpose::normal)));
        }
        send_done_event();
        serials.push_back({done_event_count, serial});
        while (serials.size() > max_remembered_serials)
        {
            serials.pop_front();
        }
    }

    void deactivated()
    {
        if (is_activated)
        {
            is_activated = false;
            send_deactivate_event();
            send_done_event();
        }
    }

    auto find_serial(uint32_t done_event_count) const -> std::optional<ms::TextInputStateSerial>
    {
        for (auto const& serial_pair : std::ranges::reverse_view(serials))
        {
            if (serial_pair.first == done_event_count)
            {
                return serial_pair.second;
            }
        }
        return std::nullopt;
    }

    void send_done_event()
    {
        done_event_count++;
        mw::InputMethodV2::send_done_event();
    }

    auto commit_string(rust::String text) -> void override
    {
        pending_change.commit_text = std::string{text};
    }

    auto set_preedit_string(rust::String text, int32_t cursor_begin, int32_t cursor_end) -> void override
    {
        pending_change.preedit_text = std::string{text};
        pending_change.preedit_cursor_begin = cursor_begin;
        pending_change.preedit_cursor_end = cursor_end;
    }

    auto delete_surrounding_text(uint32_t before_length, uint32_t after_length) -> void override
    {
        pending_change.delete_before = before_length;
        pending_change.delete_after = after_length;
    }

    auto commit(uint32_t serial) -> void override
    {
        auto const mir_serial = find_serial(serial);
        if (mir_serial)
        {
            pending_change.serial = *mir_serial;
            ctx->text_input_hub->text_changed(pending_change);
        }
        else
        {
            mir::log_warning("%s: invalid commit serial %d", "zwp_input_method_v2", serial);
        }
        pending_change = ms::TextInputChange{{}};
    }

    using mw::InputMethodV2::get_input_popup_surface;

    auto get_input_popup_surface(
        mw::Weak<mw::Surface> const&,
        rust::Box<mw::InputPopupSurfaceV2Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::InputPopupSurfaceV2> override
    {
        return std::make_shared<InputPopupSurfaceV2>(client, std::move(child_instance), child_object_id);
    }

    auto grab_keyboard(
        rust::Box<mw::InputMethodKeyboardGrabV2Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::InputMethodKeyboardGrabV2> override
    {
        if (!is_activated)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error(
                "got zwp_input_method_v2.grab_keyboard when input method was not active"));
        }
        if (current_grab_keyboard)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("tried to grab the keyboard multiple times"));
        }
        if (!seat)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("failed to resolve WlSeat grabbing keyboard"));
        }

        auto grab = std::make_shared<mf::InputMethodGrabKeyboardV2>(
            client,
            std::move(child_instance),
            child_object_id,
            *seat,
            ctx->wayland_executor,
            *ctx->event_filter);
        current_grab_keyboard = mw::make_weak<mw::InputMethodKeyboardGrabV2>(grab);
        return grab;
    }
};

class InputMethodManagerV2
    : public mw::InputMethodManagerV2
{
public:
    InputMethodManagerV2(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::InputMethodManagerV2Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<InputMethodV2Ctx> ctx)
        : mw::InputMethodManagerV2{std::move(client), std::move(instance), object_id},
          ctx{std::move(ctx)}
    {
    }

private:
    using mw::InputMethodManagerV2::get_input_method;

    auto get_input_method(
        mw::Weak<mw::Seat> const& seat,
        rust::Box<mw::InputMethodV2Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::InputMethodV2> override
    {
        auto const wl_seat = mf::WlSeat::from(seat);
        if (!wl_seat)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("failed to resolve WlSeat creating InputMethodV2"));
        }
        return std::make_shared<InputMethodV2>(client, std::move(child_instance), child_object_id, wl_seat, ctx);
    }

    std::shared_ptr<InputMethodV2Ctx> const ctx;
};
}

auto mf::create_zwp_input_method_manager_v2(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::InputMethodManagerV2Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<scene::TextInputHub> text_input_hub,
    std::shared_ptr<input::CompositeEventFilter> event_filter)
-> std::shared_ptr<mw::InputMethodManagerV2>
{
    auto ctx = std::make_shared<InputMethodV2Ctx>(
        InputMethodV2Ctx{std::move(wayland_executor), std::move(text_input_hub), std::move(event_filter)});
    return std::make_shared<InputMethodManagerV2>(std::move(client), std::move(instance), object_id, std::move(ctx));
}
