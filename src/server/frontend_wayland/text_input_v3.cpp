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
#include "weak.h"
#include "client.h"
#include "wl_seat.h"
#include "wl_surface.h"
#include <mir/executor.h>
#include <mir/scene/text_input_hub.h>
#include <mir/input/virtual_input_device.h>
#include <mir/input/device.h>
#include <mir/input/input_device_registry.h>
#include <mir/input/event_builder.h>
#include <mir/input/input_sink.h>

#include <boost/throw_exception.hpp>
#include <deque>
#include <chrono>
#include <ranges>
#include <linux/input-event-codes.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;
namespace ms = mir::scene;
namespace mi = mir::input;

namespace
{
auto wayland_to_mir_change_cause(uint32_t cause) -> ms::TextInputChangeCause
{
    switch (cause)
    {
    case mw::ZwpTextInputV3Impl::ChangeCause::input_method:
        return ms::TextInputChangeCause::input_method;

    case mw::ZwpTextInputV3Impl::ChangeCause::other:
        return ms::TextInputChangeCause::other;

    default:
        BOOST_THROW_EXCEPTION(std::invalid_argument{"Invalid text change cause " + std::to_string(cause)});
    }
}

auto wayland_to_mir_content_hint(uint32_t hint) -> ms::TextInputContentHint
{
#define WAYLAND_TO_MIR_HINT(name) \
    (hint & mw::ZwpTextInputV3Impl::ContentHint::name ? ms::TextInputContentHint::name : ms::TextInputContentHint::none)

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
    case mw::ZwpTextInputV3Impl::ContentPurpose::normal:
        return ms::TextInputContentPurpose::normal;

    case mw::ZwpTextInputV3Impl::ContentPurpose::alpha:
        return ms::TextInputContentPurpose::alpha;

    case mw::ZwpTextInputV3Impl::ContentPurpose::digits:
        return ms::TextInputContentPurpose::digits;

    case mw::ZwpTextInputV3Impl::ContentPurpose::number:
        return ms::TextInputContentPurpose::number;

    case mw::ZwpTextInputV3Impl::ContentPurpose::phone:
        return ms::TextInputContentPurpose::phone;

    case mw::ZwpTextInputV3Impl::ContentPurpose::url:
        return ms::TextInputContentPurpose::url;

    case mw::ZwpTextInputV3Impl::ContentPurpose::email:
        return ms::TextInputContentPurpose::email;

    case mw::ZwpTextInputV3Impl::ContentPurpose::name:
        return ms::TextInputContentPurpose::name;

    case mw::ZwpTextInputV3Impl::ContentPurpose::password:
        return ms::TextInputContentPurpose::password;

    case mw::ZwpTextInputV3Impl::ContentPurpose::pin:
        return ms::TextInputContentPurpose::pin;

    case mw::ZwpTextInputV3Impl::ContentPurpose::date:
        return ms::TextInputContentPurpose::date;

    case mw::ZwpTextInputV3Impl::ContentPurpose::time:
        return ms::TextInputContentPurpose::time;

    case mw::ZwpTextInputV3Impl::ContentPurpose::datetime:
        return ms::TextInputContentPurpose::datetime;

    case mw::ZwpTextInputV3Impl::ContentPurpose::terminal:
        return ms::TextInputContentPurpose::terminal;

    default:
        BOOST_THROW_EXCEPTION(std::invalid_argument{"Invalid text content purpose " + std::to_string(purpose)});
    }
}

auto mir_keyboard_action(uint32_t wayland_state) -> MirKeyboardAction
{
    switch (wayland_state)
    {
        case mw::WlKeyboardImpl::KeyState::pressed:
            return mir_keyboard_action_down;
        case mw::WlKeyboardImpl::KeyState::released:
            return mir_keyboard_action_up;
        default:
            // Protocol does not provide an appropriate error code, so throw a generic runtime_error. This will be expressed
            // to the client as an implementation error
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
}

namespace mir
{
namespace frontend
{
struct TextInputV3Ctx
{
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<scene::TextInputHub> const text_input_hub;
    std::shared_ptr<mi::InputDeviceRegistry> const device_registry;
};

class TextInputV3
    : public mw::ZwpTextInputV3Impl,
      private WlSeat::FocusListener,
      public std::enable_shared_from_this<TextInputV3>
{
public:
    TextInputV3(std::shared_ptr<TextInputV3Ctx> const& ctx, WlSeat& seat,
        std::shared_ptr<wayland_rs::Client> const& client);
    ~TextInputV3();

    /// Must be called after construction to set up the handler (requires shared_from_this())
    void init();

private:
    struct Handler : ms::TextInputChangeHandler
    {
        Handler(std::weak_ptr<TextInputV3> text_input, std::shared_ptr<Executor> const wayland_executor)
            : text_input{std::move(text_input)},
              wayland_executor{wayland_executor}
        {
        }

        void text_changed(ms::TextInputChange const& change) override
        {
            wayland_executor->spawn([text_input=text_input, change]()
                {
                    if (auto const locked = text_input.lock())
                    {
                        locked->send_text_change(change);
                    }
                });
        }

        std::weak_ptr<TextInputV3> const text_input;
        std::shared_ptr<Executor> const wayland_executor;
    };

    static size_t constexpr max_remembered_serials{10};

    std::shared_ptr<TextInputV3Ctx> const ctx;
    WlSeat& seat;
    std::shared_ptr<wayland_rs::Client> const client;
    std::shared_ptr<Handler> handler;
    mw::Weak<WlSurface> current_surface;
    /// Set to true if and only if the text input has been enabled since the last commit
    bool on_new_input_field{false};
    /// nullopt if the state is inactive, otherwise holds the pending and/or committed state
    std::optional<ms::TextInputState> pending_state;
    /// The first value is the number of commits we had received when a state was submitted to the text input hub. The
    /// second value is the serial the hub gave us for that state. When we get a change from the input method we match
    /// it's state serial to the corresponding commit count, which is the serial we send to the client. We only remember
    /// a small number of serials.
    std::deque<std::pair<uint32_t, ms::TextInputStateSerial>> state_serials;
    /// The number of commits we've received
    uint32_t commit_count{0};
    /// The keyboard device is useful when an input-method-v1 wants to simulate a key event
    std::shared_ptr<mi::VirtualInputDevice> const keyboard_device;
    std::weak_ptr<mi::Device> const device_handle;

    /// Sends the text change to the client if possible
    void send_text_change(ms::TextInputChange const& change);

    /// Returns the client serial (aka the commit count) that corresponds to the given state serial
    auto find_client_serial(ms::TextInputStateSerial state_serial) const -> std::optional<uint32_t>;

    /// From WlSeat::FocusListener
    void focus_on(std::shared_ptr<WlSurface> const& surface) override;

    /// From wayland_rs::ZwpTextInputV3Impl
    /// @{
    void enable() override;
    void disable() override;
    void set_surrounding_text(rust::String text, int32_t cursor, int32_t anchor) override;
    void set_text_change_cause(uint32_t cause) override;
    void set_content_type(uint32_t hint, uint32_t purpose) override;
    void set_cursor_rectangle(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void commit() override;
    /// @}
};
}
}

mf::TextInputManagerV3::TextInputManagerV3(
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<scene::TextInputHub> const& text_input_hub,
    std::shared_ptr<mi::InputDeviceRegistry> const& device_registry,
    std::shared_ptr<wayland_rs::Client> const& client)
    : ctx{std::shared_ptr<TextInputV3Ctx>{new TextInputV3Ctx{wayland_executor, text_input_hub, device_registry}}},
      client{client}
{
}

auto mf::TextInputManagerV3::get_text_input(wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat)
    -> std::shared_ptr<wayland_rs::ZwpTextInputV3Impl>
{
    auto const wl_seat = WlSeat::from(&seat.value());
    if (!wl_seat)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("failed to resolve WlSeat creating TextInputV3"));
    }
    auto text_input = std::make_shared<TextInputV3>(ctx, *wl_seat, client);
    text_input->init();
    return text_input;
}

mf::TextInputV3::TextInputV3(
    std::shared_ptr<TextInputV3Ctx> const& ctx,
    WlSeat& seat,
    std::shared_ptr<wayland_rs::Client> const& client)
    : ctx{ctx},
      seat{seat},
      client{client},
      keyboard_device{std::make_shared<mi::VirtualInputDevice>("virtual-keyboard", mi::DeviceCapability::keyboard)},
      device_handle{ctx->device_registry->add_device(keyboard_device)}
{
    seat.add_focus_listener(client.get(), this);
}

void mf::TextInputV3::init()
{
    handler = std::make_shared<Handler>(weak_from_this(), ctx->wayland_executor);
}

mf::TextInputV3::~TextInputV3()
{
    seat.remove_focus_listener(client.get(), this);
    ctx->device_registry->remove_device(keyboard_device);
    ctx->text_input_hub->deactivate_handler(handler);
}

void mf::TextInputV3::send_text_change(ms::TextInputChange const& change)
{
    auto const client_serial = find_client_serial(change.serial);
    if (!pending_state || !current_surface || !client_serial)
    {
        // We are no longer enabled or we don't have a valid serial
        return;
    }
    if (change.keysym)
    {
        auto nano = std::chrono::milliseconds{change.keysym->time};
        keyboard_device->if_started_then([&](input::InputSink* sink, input::EventBuilder* builder)
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
            change.preedit_text.value_or(""),
            change.preedit_text.has_value(),
            change.preedit_cursor_begin.value_or(0),
            change.preedit_cursor_end.value_or(0));
    }
    if (change.delete_before || change.delete_after)
    {
        send_delete_surrounding_text_event(change.delete_before.value_or(0), change.delete_after.value_or(0));
    }
    if(change.commit_text)
    {
        send_commit_string_event(change.commit_text.value(), true);
    }
    send_done_event(client_serial.value());
}

auto mf::TextInputV3::find_client_serial(ms::TextInputStateSerial state_serial) const -> std::optional<uint32_t>
{
    // Loop in reverse order because the serial we're looking for will generally be at the end
    for (auto const& serial_pair : std::ranges::reverse_view(state_serials))
    {
        if (serial_pair.second == state_serial)
        {
            return serial_pair.first;
        }
    }
    return std::nullopt;
}

void mf::TextInputV3::focus_on(std::shared_ptr<WlSurface> const& surface)
{
    if (current_surface)
    {
        send_leave_event(current_surface.value().shared_from_this());
    }
    current_surface = mw::Weak<WlSurface>(surface);
    if (surface)
    {
        send_enter_event(surface);
    }
    else
    {
        disable();
        ctx->text_input_hub->deactivate_handler(handler);
    }
}

void mf::TextInputV3::enable()
{
    if (current_surface)
    {
        on_new_input_field = true;
        pending_state.emplace();
    }
}

void mf::TextInputV3::disable()
{
    on_new_input_field = false;
    pending_state.reset();
}

void mf::TextInputV3::set_surrounding_text(rust::String text, int32_t cursor, int32_t anchor)
{
    if (pending_state)
    {
        pending_state->surrounding_text = std::string(text);
        pending_state->cursor = cursor;
        pending_state->anchor = anchor;
    }
}

void mf::TextInputV3::set_text_change_cause(uint32_t cause)
{
    if (pending_state)
    {
        pending_state->change_cause = wayland_to_mir_change_cause(cause);
    }
}

void mf::TextInputV3::set_content_type(uint32_t hint, uint32_t purpose)
{
    if (pending_state)
    {
        pending_state->content_hint.emplace(wayland_to_mir_content_hint(hint));
        pending_state->content_purpose = wayland_to_mir_content_purpose(purpose);
    }
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
