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

#include "text_input_v2.h"

#include "wl_seat.h"
#include "wl_surface.h"
#include "mir/executor.h"
#include "mir/scene/text_input_hub.h"

#include <boost/throw_exception.hpp>
#include <mir/wayland/client.h>
#include <deque>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace ms = mir::scene;

namespace
{
auto wayland_to_mir_content_hint(uint32_t hint) -> ms::TextInputContentHint
{
#define WAYLAND_TO_MIR_HINT(wl_name, mir_name) \
    (hint & mw::TextInputV2::ContentHint::wl_name ? \
        ms::TextInputContentHint::mir_name : \
        ms::TextInputContentHint::none)

    return
        WAYLAND_TO_MIR_HINT(auto_completion, completion) |
        WAYLAND_TO_MIR_HINT(auto_correction, spellcheck) |
        WAYLAND_TO_MIR_HINT(auto_capitalization, auto_capitalization) |
        WAYLAND_TO_MIR_HINT(lowercase, lowercase) |
        WAYLAND_TO_MIR_HINT(uppercase, uppercase) |
        WAYLAND_TO_MIR_HINT(titlecase, titlecase) |
        WAYLAND_TO_MIR_HINT(hidden_text, hidden_text) |
        WAYLAND_TO_MIR_HINT(sensitive_data, sensitive_data) |
        WAYLAND_TO_MIR_HINT(latin, latin) |
        WAYLAND_TO_MIR_HINT(multiline, multiline);

#undef WAYLAND_TO_MIR_HINT
}

auto wayland_to_mir_content_purpose(uint32_t purpose) -> ms::TextInputContentPurpose
{
    switch (purpose)
    {
    case mw::TextInputV2::ContentPurpose::normal:
        return ms::TextInputContentPurpose::normal;

    case mw::TextInputV2::ContentPurpose::alpha:
        return ms::TextInputContentPurpose::alpha;

    case mw::TextInputV2::ContentPurpose::digits:
        return ms::TextInputContentPurpose::digits;

    case mw::TextInputV2::ContentPurpose::number:
        return ms::TextInputContentPurpose::number;

    case mw::TextInputV2::ContentPurpose::phone:
        return ms::TextInputContentPurpose::phone;

    case mw::TextInputV2::ContentPurpose::url:
        return ms::TextInputContentPurpose::url;

    case mw::TextInputV2::ContentPurpose::email:
        return ms::TextInputContentPurpose::email;

    case mw::TextInputV2::ContentPurpose::name:
        return ms::TextInputContentPurpose::name;

    case mw::TextInputV2::ContentPurpose::password:
        return ms::TextInputContentPurpose::password;

    case mw::TextInputV2::ContentPurpose::date:
        return ms::TextInputContentPurpose::date;

    case mw::TextInputV2::ContentPurpose::time:
        return ms::TextInputContentPurpose::time;

    case mw::TextInputV2::ContentPurpose::datetime:
        return ms::TextInputContentPurpose::datetime;

    case mw::TextInputV2::ContentPurpose::terminal:
        return ms::TextInputContentPurpose::terminal;

    default:
        BOOST_THROW_EXCEPTION(std::invalid_argument{"Invalid text content purpose " + std::to_string(purpose)});
    }
}

enum TextInputV2UpdateState
{
    text_input_v2_update_state_change,
    text_input_v2_update_state_full,
    text_input_v2_update_state_reset,
    text_input_v2_update_state_enter
};

}

namespace mir
{
namespace frontend
{
struct TextInputV2Ctx
{
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<scene::TextInputHub> const text_input_hub;
};

class TextInputManagerV2Global
    : public wayland::TextInputManagerV2::Global
{
public:
    TextInputManagerV2Global(wl_display* display, std::shared_ptr<TextInputV2Ctx> const& ctx);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<TextInputV2Ctx> const ctx;
};

class TextInputManagerV2
    : public wayland::TextInputManagerV2
{
public:
    TextInputManagerV2(wl_resource* resource, std::shared_ptr<TextInputV2Ctx> const& ctx);

private:
    void get_text_input(struct wl_resource* id, struct wl_resource* seat) override;

    std::shared_ptr<TextInputV2Ctx> const ctx;
};

class TextInputV2
    : public wayland::TextInputV2,
      private WlSeat::FocusListener
{
public:
    TextInputV2(wl_resource* resource, std::shared_ptr<TextInputV2Ctx> const& ctx, WlSeat& seat);
    ~TextInputV2();

private:
    struct Handler : ms::TextInputChangeHandler
    {
        Handler(TextInputV2* const text_input, std::shared_ptr<Executor> const wayland_executor)
            : text_input{text_input},
              wayland_executor{wayland_executor}
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

        wayland::Weak<TextInputV2> const text_input;
        std::shared_ptr<Executor> const wayland_executor;
    };

    static size_t constexpr max_remembered_serials{10};

    std::shared_ptr<TextInputV2Ctx> const ctx;
    WlSeat& seat;
    std::shared_ptr<Handler> const handler;
    wayland::Weak<WlSurface> current_surface;
    /// Set to true if and only if the text input has been enabled since the last commit
    bool on_new_input_field{false};
    /// nullopt if the state is inactive, otherwise holds the pending and/or committed state
    std::optional<ms::TextInputState> pending_state;
    struct SerialPair
    {
        uint32_t client_serial;
        ms::TextInputStateSerial hub_serial;
    };
    std::deque<SerialPair> state_serials;

    /// Sends the text change to the client if possible
    void send_text_change(ms::TextInputChange const& change);

    /// Returns the client serial (aka the commit count) that corresponds to the given state serial
    auto find_client_serial(ms::TextInputStateSerial state_serial) const -> std::optional<uint32_t>;

    /// From WlSeat::FocusListener
    void focus_on(WlSurface* surface) override;

    /// From wayland::TextInputV2
    /// @{
    void enable(wl_resource* surface) override;
    void disable(wl_resource* surface) override;
    void show_input_panel() override;
    void hide_input_panel() override;
    void set_surrounding_text(std::string const& text, int32_t cursor, int32_t anchor) override;
    void set_content_type(uint32_t hint, uint32_t purpose) override;
    void set_cursor_rectangle(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_preferred_language(std::string const& language) override;
    void update_state(uint32_t serial, uint32_t reason) override;
    /// @}
};
}
}

auto mf::create_text_input_manager_v2(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<scene::TextInputHub> const& text_input_hub)
-> std::shared_ptr<mw::TextInputManagerV2::Global>
{
    auto ctx = std::shared_ptr<TextInputV2Ctx>{new TextInputV2Ctx{wayland_executor, text_input_hub}};
    return std::make_shared<TextInputManagerV2Global>(display, std::move(ctx));
}

mf::TextInputManagerV2Global::TextInputManagerV2Global(
    wl_display* display,
    std::shared_ptr<TextInputV2Ctx> const& ctx)
    : Global{display, Version<1>()},
      ctx{ctx}
{
}

void mf::TextInputManagerV2Global::bind(wl_resource* new_resource)
{
    new TextInputManagerV2{new_resource, ctx};
}

mf::TextInputManagerV2::TextInputManagerV2(
    wl_resource* resource,
    std::shared_ptr<TextInputV2Ctx> const& ctx)
    : wayland::TextInputManagerV2{resource, Version<1>()},
      ctx{ctx}
{
}

void mf::TextInputManagerV2::get_text_input(struct wl_resource* id, struct wl_resource* seat)
{
    auto const wl_seat = WlSeat::from(seat);
    if (!wl_seat)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("failed to resolve WlSeat creating TextInputV2"));
    }
    new TextInputV2{id, ctx, *wl_seat};
}

mf::TextInputV2::TextInputV2(
    wl_resource* resource,
    std::shared_ptr<TextInputV2Ctx> const& ctx,
    WlSeat& seat)
    : wayland::TextInputV2{resource, Version<1>()},
      ctx{ctx},
      seat{seat},
      handler{std::make_shared<Handler>(this, ctx->wayland_executor)}
{
    seat.add_focus_listener(client, this);
}

mf::TextInputV2::~TextInputV2()
{
    seat.remove_focus_listener(client, this);
    ctx->text_input_hub->deactivate_handler(handler);
}

void mf::TextInputV2::send_text_change(ms::TextInputChange const& change)
{
    auto const client_serial = find_client_serial(change.serial);
    if (!pending_state || !current_surface || !client_serial)
    {
        // We are no longer enabled or we don't have a valid serial
        return;
    }
    if (change.preedit_style)
    {
        send_preedit_styling_event(
            change.preedit_style->index,
            change.preedit_style->length,
            change.preedit_style->style);
    }
    if (change.keysym)
    {
        send_keysym_event(
            change.keysym->time,
            change.keysym->sym,
            change.keysym->state,
            change.keysym->modifiers);
    }
    if (change.modifier_map)
    {
        send_modifiers_map_event(change.modifier_map.value().data());
    }
    if (change.direction)
    {
        send_text_direction_event(change.direction.value());
    }
    if (change.preedit_text || change.preedit_cursor_begin || change.preedit_cursor_end)
    {
        send_preedit_cursor_event(change.preedit_cursor_begin.value_or(0));
        send_preedit_string_event(change.preedit_text.value_or(""), change.preedit_commit.value_or(""));
    }
    if (change.cursor_position)
    {
        send_cursor_position_event(change.cursor_position->index, change.cursor_position->anchor);
    }
    if (change.delete_before || change.delete_after)
    {
        send_delete_surrounding_text_event(change.delete_before.value_or(0), change.delete_after.value_or(0));
    }
    if(change.commit_text)
    {
        send_commit_string_event(change.commit_text.value());
    }
}

auto mf::TextInputV2::find_client_serial(ms::TextInputStateSerial state_serial) const -> std::optional<uint32_t>
{
    // Loop in reverse order because the serial we're looking for will generally be at the end
    for (auto it = state_serials.rbegin(); it != state_serials.rend(); it++)
    {
        if (it->hub_serial == state_serial)
        {
            return it->client_serial;
        }
    }
    return std::nullopt;
}

void mf::TextInputV2::focus_on(WlSurface* surface)
{
    if (current_surface)
    {
        auto const serial = client->next_serial(nullptr);
        send_leave_event(serial, current_surface.value().resource);
    }
    current_surface = mw::make_weak(surface);
    if (surface)
    {
        auto const serial = client->next_serial(nullptr);
        send_enter_event(serial, surface->resource);
    }
    else
    {
        disable(nullptr);
        ctx->text_input_hub->deactivate_handler(handler);
    }
}

void mf::TextInputV2::enable(wl_resource*)
{
    if (current_surface)
    {
        on_new_input_field = true;
        pending_state.emplace();
    }
}

void mf::TextInputV2::disable(wl_resource*)
{
    on_new_input_field = false;
    pending_state.reset();
}

void mf::TextInputV2::show_input_panel()
{
    ctx->text_input_hub->show_input_panel();
}

void mf::TextInputV2::hide_input_panel()
{
    ctx->text_input_hub->hide_input_panel();
}

void mf::TextInputV2::set_surrounding_text(std::string const& text, int32_t cursor, int32_t anchor)
{
    if (pending_state)
    {
        pending_state->surrounding_text = text;
        pending_state->cursor = cursor;
        pending_state->anchor = anchor;
    }
}

void mf::TextInputV2::set_content_type(uint32_t hint, uint32_t purpose)
{
    if (pending_state)
    {
        pending_state->content_hint.emplace(wayland_to_mir_content_hint(hint));
        pending_state->content_purpose = wayland_to_mir_content_purpose(purpose);
    }
}

void mf::TextInputV2::set_cursor_rectangle(int32_t x, int32_t y, int32_t width, int32_t height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void mf::TextInputV2::set_preferred_language(std::string const&)
{
    // Ignored, input methods decide language for themselves
}

void mf::TextInputV2::update_state(uint32_t client_serial, uint32_t reason)
{
    if (pending_state && current_surface)
    {
        // We can attribute any simple change to the input_method while reserving any other change to "other"
        switch (reason)
        {
            case text_input_v2_update_state_change:
                pending_state->change_cause = mir::scene::TextInputChangeCause::input_method;
                break;
            default:
                pending_state->change_cause = mir::scene::TextInputChangeCause::other;
                break;
        }

        auto const hub_serial = ctx->text_input_hub->set_handler_state(handler, on_new_input_field, *pending_state);
        state_serials.push_back({client_serial, hub_serial});
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
