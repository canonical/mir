/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "xdg_activation_unstable_v1.h"

#include <mir/events/keyboard_event.h>
#include <mir/input/keyboard_observer.h>
#include <mir/main_loop.h>
#include <mir/scene/null_session_listener.h>
#include <mir/scene/session.h>
#include <mir/scene/session_coordinator.h>
#include <mir/scene/session_listener.h>
#include <mir/shell/shell.h>
#include <mir/shell/token_authority.h>
#include <mir/log.h>
#include "protocol_error.h"
#include "wl_seat.h"
#include "wl_surface.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mi = mir::input;

using Token = msh::TokenAuthority::Token;

// XdgActivationV1State observers

class mf::XdgActivationV1State::KeyboardObserver : public mi::KeyboardObserver
{
public:
    KeyboardObserver(XdgActivationV1State* state) : state{state} {}

    void keyboard_event(std::shared_ptr<MirEvent const> const& event) override
    {
        // If we encounter a key press event, then we invalidate pending activation tokens
        if (event->type() != mir_event_type_input)
            return;

        auto input_event = event->to_input();
        if (input_event->input_type() != mir_input_event_type_key)
            return;

        auto keyboard_event = input_event->to_keyboard();
        if (keyboard_event->action() == mir_keyboard_action_down)
            state->invalidate_all();
    }

    void keyboard_focus_set(std::shared_ptr<mi::Surface> const&) override {}

private:
    XdgActivationV1State* const state;
};

class mf::XdgActivationV1State::SessionListener : public ms::NullSessionListener
{
public:
    SessionListener(XdgActivationV1State* state) : state{state} {}

    void focused(std::shared_ptr<ms::Session> const& session) override
    {
        focused_session = session;
        state->invalidate_if_not_from_session(session);
    }

    void unfocused() override
    {
        focused_session = nullptr;
        state->invalidate_all();
    }

    bool is_focused_session(std::shared_ptr<ms::Session> const& other) const
    {
        return focused_session == other;
    }

private:
    XdgActivationV1State* const state;
    std::shared_ptr<ms::Session> focused_session;
};

// XdgActivationV1State

mf::XdgActivationV1State::XdgActivationV1State(
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<MainLoop> const& main_loop,
    std::shared_ptr<ObserverRegistrar<mi::KeyboardObserver>> const& keyboard_observer_registrar,
    Executor& wayland_executor,
    std::shared_ptr<msh::TokenAuthority> const& token_authority)
    : shell_{shell},
      session_coordinator_{session_coordinator},
      main_loop_{main_loop},
      keyboard_observer_registrar{keyboard_observer_registrar},
      keyboard_observer{std::make_shared<KeyboardObserver>(this)},
      session_listener{std::make_shared<SessionListener>(this)},
      token_authority_{token_authority}
{
    keyboard_observer_registrar->register_interest(keyboard_observer, wayland_executor);
    session_coordinator->add_listener(session_listener);
}

mf::XdgActivationV1State::~XdgActivationV1State()
{
    keyboard_observer_registrar->unregister_interest(*keyboard_observer);
    session_coordinator_->remove_listener(session_listener);
}

auto mf::XdgActivationV1State::get_token_data(Token const& token) -> std::shared_ptr<XdgActivationTokenData>
{
    std::lock_guard guard(pending_tokens_mutex);

    auto iter = std::ranges::find_if(
        pending_tokens,
        [&](auto const& pending_token)
        {
            return pending_token->token == token;
        });

    if (iter != pending_tokens.end())
        return *iter;

    return nullptr;
}

auto mf::XdgActivationV1State::create_token(
    std::optional<uint32_t> serial,
    std::optional<mw::Weak<mw::Seat>> seat,
    std::shared_ptr<ms::Session> session) -> std::shared_ptr<XdgActivationTokenData>
{
    if (!session_listener->is_focused_session(session))
        return std::make_shared<XdgActivationTokenData>(
            token_authority_->get_bogus_token(), session, serial, seat);

    auto generated = token_authority_->issue_token(
        [this](auto const& token)
        {
            this->remove_token(token);
        });

    auto token = std::make_shared<XdgActivationTokenData>(generated, session, serial, seat);

    {
        std::lock_guard guard(pending_tokens_mutex);
        return pending_tokens.emplace_back(token);
    }
}

void mf::XdgActivationV1State::remove_token(Token const& token)
{
    std::lock_guard guard(pending_tokens_mutex);

    std::erase_if(pending_tokens,
        [&](auto const& pending_token)
        {
            return pending_token->token == token;
        });
}

void mf::XdgActivationV1State::invalidate_all()
{
    token_authority_->revoke_all_tokens();
}

void mf::XdgActivationV1State::invalidate_if_not_from_session(std::shared_ptr<ms::Session> const& session)
{
    auto const invalid_session = [&session](auto const& token_data)
    {
        return token_data->session.expired() || token_data->session.lock() != session;
    };

    decltype(pending_tokens) to_invalidate;

    {
        std::lock_guard guard(pending_tokens_mutex);
        for (auto const& token : pending_tokens | std::ranges::views::filter(invalid_session))
            to_invalidate.push_back(token);
    }

    for (auto const& token : to_invalidate)
        token_authority_->revoke_token(token->token);
}

// Per-bind protocol objects

namespace mir
{
namespace frontend
{
namespace
{
class XdgActivationTokenV1 : public mw::XdgActivationTokenV1
{
public:
    XdgActivationTokenV1(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::XdgActivationTokenV1Middleware> instance,
        uint32_t object_id,
        XdgActivationV1State& state)
        : mw::XdgActivationTokenV1{std::move(client), std::move(instance), object_id},
          state{state},
          session{this->client->client_session()}
    {
    }

private:
    using mw::XdgActivationTokenV1::set_serial;
    void set_serial(uint32_t serial_, mw::Weak<mw::Seat> const& seat_) override
    {
        serial = serial_;
        seat = seat_;
    }

    void set_app_id(rust::String app_id) override
    {
        // TODO: This is the application id of the surface that is coming up.
        //  Until it presents itself as a problem, we will ignore it for now.
        (void)app_id;
    }

    using mw::XdgActivationTokenV1::set_surface;
    void set_surface(mw::Weak<mw::Surface> const& surface) override
    {
        // TODO: This is the requesting surface. Until it presents itself
        //  as a problem, we will ignore it for now.
        (void)surface;
    }

    void commit() override
    {
        if (used)
        {
            throw mw::ProtocolError{
                object_id(),
                Error::already_used,
                "The activation token has already been used"};
        }

        used = true;
        token = state.create_token(serial, seat, session);
        send_done_event(std::string(token->token));
    }

    XdgActivationV1State& state;
    bool used{false};
    std::optional<uint32_t> serial;
    std::optional<mw::Weak<mw::Seat>> seat;
    std::shared_ptr<XdgActivationTokenData> token;
    std::shared_ptr<ms::Session> session;
};

class XdgActivationV1Instance : public mw::XdgActivationV1
{
public:
    XdgActivationV1Instance(
        std::shared_ptr<mw::Client> client,
        rust::Box<mw::XdgActivationV1Middleware> instance,
        uint32_t object_id,
        XdgActivationV1State& state)
        : mw::XdgActivationV1{std::move(client), std::move(instance), object_id},
          state{state}
    {
    }

private:
    auto get_activation_token(
        rust::Box<mw::XdgActivationTokenV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mw::XdgActivationTokenV1> override
    {
        return std::make_shared<XdgActivationTokenV1>(client, std::move(child_instance), child_object_id, state);
    }

    using mw::XdgActivationV1::activate;
    void activate(rust::String string_token, mw::Weak<mw::Surface> const& surface) override
    {
        // This function handles requests from clients for activation.
        // This will fail if the client cannot find their token in the list.

        auto token = state.token_authority()->get_token_for_string(std::string(string_token));

        // Invalid token, ignore it
        if (!token)
            return;

        // Grab a reference to the token data before removing it
        //
        // token_data can be null if the token was issued by something other
        // than `XdgActivationV1` (`ExternalClientLauncher` for example)
        auto token_data = state.get_token_data(*token);

        // Calls `remove_token` since we set that up at token creation time
        state.token_authority()->revoke_token(*token);

        state.main_loop()->enqueue(this, [
            surface,
            shell=state.shell(),
            xdg_token=token_data,
            client=client,
            session_coordinator=state.session_coordinator()]
        {
            if (xdg_token && xdg_token->session.expired())
            {
                mir::log_error("XdgActivationV1::activate requesting session has expired");
                return;
            }

            auto const wl_surface = mw::Surface::from<mf::WlSurface>(surface);
            if (!wl_surface)
            {
                mir::log_error("XdgActivationV1::activate wl_surface not found");
                return;
            }

            auto const scene_surface_opt = wl_surface->scene_surface();
            if (!scene_surface_opt)
            {
                mir::log_error("XdgActivationV1::activate scene_surface_opt not found");
                return;
            }

            // In the event of a failure, we send 'done' with a new, invalid token which cannot be used later.
            if (xdg_token && xdg_token->seat && xdg_token->serial)
            {
                // First, assert that the seat still exists.
                auto const wl_seat = mf::WlSeat::from(xdg_token->seat.value());
                if (!wl_seat)
                    mir::log_warning("XdgActivationTokenV1::activate wl_seat not found");

                auto event = client->event_for(xdg_token->serial.value());
                if (event == std::nullopt)
                    mir::log_warning("XdgActivationTokenV1::activate serial not found");
            }

            auto const& scene_surface = scene_surface_opt.value();
            auto const now = std::chrono::steady_clock::now().time_since_epoch();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
            shell->raise_surface(wl_surface->session, scene_surface, ns);
        });
    }

    XdgActivationV1State& state;
};
}
}
}

auto mf::create_xdg_activation_v1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::XdgActivationV1Middleware> instance,
    uint32_t object_id,
    XdgActivationV1State& state) -> std::shared_ptr<mw::XdgActivationV1>
{
    return std::make_shared<XdgActivationV1Instance>(std::move(client), std::move(instance), object_id, state);
}
