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

#include "text_input_v3.h"

#include "wayland.h"
#include "wl_seat.h"
#include "wl_surface.h"
#include <mir/executor.h>
#include <mir/input/device.h>
#include <mir/input/event_builder.h>
#include <mir/input/input_device_registry.h>
#include <mir/input/input_sink.h>
#include <mir/input/virtual_input_device.h>
#include <mir/scene/text_input_hub.h>

#include <boost/throw_exception.hpp>
#include <chrono>
#include <deque>
#include <linux/input-event-codes.h>
#include <ranges>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace ms = mir::scene;
namespace mi = mir::input;

namespace
{
auto wayland_to_mir_change_cause(uint32_t cause) -> ms::TextInputChangeCause
{
    switch (cause)
    {
    case mw::TextInputV3::ChangeCause::input_method:
        return ms::TextInputChangeCause::input_method;

    case mw::TextInputV3::ChangeCause::other:
        return ms::TextInputChangeCause::other;

    default:
        BOOST_THROW_EXCEPTION(std::invalid_argument{"Invalid text change cause " + std::to_string(cause)});
    }
}

auto wayland_to_mir_content_hint(uint32_t hint) -> ms::TextInputContentHint
{
#define WAYLAND_TO_MIR_HINT(name) \
    (hint & mw::TextInputV3::ContentHint::name ? ms::TextInputContentHint::name : ms::TextInputContentHint::none)

    return
        WAYLAND_TO_MIR_HINT(completion) |
        WAYLAND_TO_MIR_HINT(spellcheck) |
        WAYLAND_TO_MIR_HINT(auto_capitalization) |
        WAYLAND_TO_MIR_HINT(lowercase) |
        WAYLAND_TO_MIR_HINT(uppercase) |
        WAYLAND_TO_MIR_HINT(titlecase) |
        WAYLAND_TO_MIR_HINT(hidden_text) |
        WAYLAND_TO_MIR_HINT(sensitive_data) |
        WAYLAND_TO_MIR_HINT(latin) |
        WAYLAND_TO_MIR_HINT(multiline);

#undef WAYLAND_TO_MIR_HINT
}

auto wayland_to_mir_content_purpose(uint32_t purpose) -> ms::TextInputContentPurpose
{
    switch (purpose)
    {
    case mw::TextInputV3::ContentPurpose::normal:
        return ms::TextInputContentPurpose::normal;
    case mw::TextInputV3::ContentPurpose::alpha:
        return ms::TextInputContentPurpose::alpha;
    case mw::TextInputV3::ContentPurpose::digits:
        return ms::TextInputContentPurpose::digits;
    case mw::TextInputV3::ContentPurpose::number:
        return ms::TextInputContentPurpose::number;
    case mw::TextInputV3::ContentPurpose::phone:
        return ms::TextInputContentPurpose::phone;
    case mw::TextInputV3::ContentPurpose::url:
        return ms::TextInputContentPurpose::url;
    case mw::TextInputV3::ContentPurpose::email:
        return ms::TextInputContentPurpose::email;
    case mw::TextInputV3::ContentPurpose::name:
        return ms::TextInputContentPurpose::name;
    case mw::TextInputV3::ContentPurpose::password:
        return ms::TextInputContentPurpose::password;
    case mw::TextInputV3::ContentPurpose::pin:
        return ms::TextInputContentPurpose::pin;
    case mw::TextInputV3::ContentPurpose::date:
        return ms::TextInputContentPurpose::date;
    case mw::TextInputV3::ContentPurpose::time:
        return ms::TextInputContentPurpose::time;
    case mw::TextInputV3::ContentPurpose::datetime:
        return ms::TextInputContentPurpose::datetime;
    case mw::TextInputV3::ContentPurpose::terminal:
        return ms::TextInputContentPurpose::terminal;
    default:
        BOOST_THROW_EXCEPTION(std::invalid_argument{"Invalid text content purpose " + std::to_string(purpose)});
    }
}

auto mir_keyboard_action(uint32_t wayland_state) -> MirKeyboardAction
{
    switch (wayland_state)
    {
        case mw::Keyboard::KeyState::pressed:
            return mir_keyboard_action_down;
        case mw::Keyboard::KeyState::released:
            return mir_keyboard_action_up;
        default:
            BOOST_THROW_EXCEPTION(std::runtime_error(
                "Invalid virtual keyboard key state " + std::to_string(wayland_state)));
    }
}

uint32_t sym_to_scan_code(uint32_t sym)
{
    switch (sym)
    {
        case XKB_KEY_BackSpace:
            return KEY_BACKSPACE;
        case XKB_KEY_Return:
            return KEY_ENTER;
        case XKB_KEY_Left:
            return KEY_LEFT;
        case XKB_KEY_Right:
            return KEY_RIGHT;
        case XKB_KEY_Up:
            return KEY_UP;
        case XKB_KEY_Down:
            return KEY_DOWN;
        default:
            return KEY_UNKNOWN;
    }
}

struct TextInputV3Ctx
{
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<ms::TextInputHub> const text_input_hub;
    std::shared_ptr<mi::InputDeviceRegistry> const device_registry;
};

class TextInputV3
    : public mw::TextInputV3,
      private mf::WlSeat::FocusListener
{
public:
    TextInputV3(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::TextInputV3Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<TextInputV3Ctx> const& ctx,
        mf::WlSeat& seat)
        : mw::TextInputV3{std::move(client), std::move(instance), object_id},
          ctx{ctx},
          seat{seat},
          handler{std::make_shared<Handler>(this, ctx->wayland_executor)},
          keyboard_device{std::make_shared<mi::VirtualInputDevice>("virtual-keyboard", mi::DeviceCapability::keyboard)},
          device_handle{ctx->device_registry->add_device(keyboard_device)}
    {
        seat.add_focus_listener(this->client.get(), this);
    }

    ~TextInputV3() override
    {
        seat.remove_focus_listener(client.get(), this);
        ctx->device_registry->remove_device(keyboard_device);
        ctx->text_input_hub->deactivate_handler(handler);
    }

private:
    struct Handler : ms::TextInputChangeHandler
    {
        Handler(TextInputV3* const text_input, std::shared_ptr<mir::Executor> wayland_executor)
            : text_input{text_input},
              wayland_executor{std::move(wayland_executor)}
        {
        }

        void text_changed(ms::TextInputChange const& change) override
        {
            wayland_executor->spawn([text_input=text_input, change]()
                {
                    if (text_input)
                    {
                        text_input.value().send_text_change(change);
                    }
                });
        }

        mw::Weak<TextInputV3> const text_input;
        std::shared_ptr<mir::Executor> const wayland_executor;
    };

    static size_t constexpr max_remembered_serials{10};

    std::shared_ptr<TextInputV3Ctx> const ctx;
    mf::WlSeat& seat;
    std::shared_ptr<Handler> const handler;
    mw::Weak<mf::WlSurface> current_surface;
    bool on_new_input_field{false};
    std::optional<ms::TextInputState> pending_state;
    std::deque<std::pair<uint32_t, ms::TextInputStateSerial>> state_serials;
    uint32_t commit_count{0};
    std::shared_ptr<mi::VirtualInputDevice> const keyboard_device;
    std::weak_ptr<mi::Device> const device_handle;

    void send_text_change(ms::TextInputChange const& change)
    {
        auto const client_serial = find_client_serial(change.serial);
        if (!pending_state || !current_surface || !client_serial)
        {
            return;
        }
        if (change.keysym)
        {
            auto nano = std::chrono::milliseconds{change.keysym->time};
            keyboard_device->if_started_then([&](mi::InputSink* sink, mi::EventBuilder* builder)
                {
                    auto state = change.keysym->state;
                    auto scan_code = sym_to_scan_code(change.keysym->sym);
                    auto key_event = builder->key_event(nano, mir_keyboard_action(state), 0, scan_code);
                    sink->handle_input(std::move(key_event));
                });
        }
        if (change.preedit_text || change.preedit_cursor_begin || change.preedit_cursor_end)
        {
            send_preedit_string_event(
                change.preedit_text,
                change.preedit_cursor_begin.value_or(0),
                change.preedit_cursor_end.value_or(0));
        }
        if (change.delete_before || change.delete_after)
        {
            send_delete_surrounding_text_event(change.delete_before.value_or(0), change.delete_after.value_or(0));
        }
        if (change.commit_text)
        {
            send_commit_string_event(change.commit_text.value());
        }
        send_done_event(client_serial.value());
    }

    auto find_client_serial(ms::TextInputStateSerial state_serial) const -> std::optional<uint32_t>
    {
        for (auto const& serial_pair : std::ranges::reverse_view(state_serials))
        {
            if (serial_pair.second == state_serial)
            {
                return serial_pair.first;
            }
        }
        return std::nullopt;
    }

    void focus_on(mf::WlSurface* surface) override
    {
        if (current_surface)
        {
            send_leave_event(as_surface_ptr(&current_surface.value()));
        }
        current_surface = mw::make_weak(surface);
        if (surface)
        {
            send_enter_event(as_surface_ptr(surface));
        }
        else
        {
            disable();
            ctx->text_input_hub->deactivate_handler(handler);
        }
    }

    auto enable() -> void override
    {
        if (current_surface)
        {
            on_new_input_field = true;
            pending_state.emplace();
        }
    }

    auto disable() -> void override
    {
        on_new_input_field = false;
        pending_state.reset();
    }

    auto set_surrounding_text(rust::String text, int32_t cursor, int32_t anchor) -> void override
    {
        if (pending_state)
        {
            pending_state->surrounding_text = std::string{text};
            pending_state->cursor = cursor;
            pending_state->anchor = anchor;
        }
    }

    auto set_text_change_cause(uint32_t cause) -> void override
    {
        if (pending_state)
        {
            pending_state->change_cause = wayland_to_mir_change_cause(cause);
        }
    }

    auto set_content_type(uint32_t hint, uint32_t purpose) -> void override
    {
        if (pending_state)
        {
            pending_state->content_hint.emplace(wayland_to_mir_content_hint(hint));
            pending_state->content_purpose = wayland_to_mir_content_purpose(purpose);
        }
    }

    auto set_cursor_rectangle(int32_t x, int32_t y, int32_t width, int32_t height) -> void override
    {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
    }

    auto commit() -> void override
    {
        commit_count++;
        if (pending_state && current_surface)
        {
            auto const serial = ctx->text_input_hub->set_handler_state(handler, on_new_input_field, *pending_state);
            state_serials.push_back({commit_count, serial});
            while (state_serials.size() > max_remembered_serials)
            {
                state_serials.pop_front();
            }
        }
        else
        {
            ctx->text_input_hub->deactivate_handler(handler);
        }
        on_new_input_field = false;
    }
};

class TextInputManagerV3 : public mw::TextInputManagerV3
{
public:
    TextInputManagerV3(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::TextInputManagerV3Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<TextInputV3Ctx> ctx)
        : mw::TextInputManagerV3{std::move(client), std::move(instance), object_id},
          ctx{std::move(ctx)}
    {
    }

private:
    using mw::TextInputManagerV3::get_text_input;

    auto get_text_input(
        mw::Weak<mw::Seat> const& seat,
        rust::Box<mw::TextInputV3Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::TextInputV3> override
    {
        auto const wl_seat = mf::WlSeat::from(seat);
        if (!wl_seat)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("failed to resolve WlSeat creating TextInputV3"));
        }
        return std::make_shared<TextInputV3>(client, std::move(child_instance), child_object_id, ctx, *wl_seat);
    }

    std::shared_ptr<TextInputV3Ctx> const ctx;
};
}

auto mf::create_zwp_text_input_manager_v3(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::TextInputManagerV3Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<scene::TextInputHub> text_input_hub,
    std::shared_ptr<mi::InputDeviceRegistry> device_registry)
-> std::shared_ptr<mw::TextInputManagerV3>
{
    auto ctx = std::make_shared<TextInputV3Ctx>(
        TextInputV3Ctx{std::move(wayland_executor), std::move(text_input_hub), std::move(device_registry)});
    return std::make_shared<TextInputManagerV3>(std::move(client), std::move(instance), object_id, std::move(ctx));
}
