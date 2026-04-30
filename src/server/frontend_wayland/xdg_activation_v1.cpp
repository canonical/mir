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

#include "xdg_activation_v1.h"
#include "protocol_error.h"

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
#include "wl_seat.h"
#include "wl_surface.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mi = mir::input;

using Token = msh::TokenAuthority::Token;

namespace mir
{
namespace frontend
{
struct XdgActivationTokenData
{
    XdgActivationTokenData(
        Token const& token,
        std::shared_ptr<ms::Session> session,
        std::optional<uint32_t> serial,
        wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat) :
        token{token},
        session{session},
        serial{serial},
        seat{seat}
    {
    }

    Token const token;
    std::weak_ptr<ms::Session> session;
    std::optional<uint32_t> serial;
     wayland_rs::Weak<wayland_rs::WlSeatImpl> seat;
};

class XdgActivationV1 : public XdgActivationV1Global
{
public:
    XdgActivationV1(
        std::shared_ptr<msh::Shell> const& shell,
        std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
        std::shared_ptr<MainLoop> const& main_loop,
        std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const& keyboard_observer_registrar,
        Executor& wayland_executor,
        std::shared_ptr<msh::TokenAuthority> const& token_authority);
    ~XdgActivationV1();

    std::shared_ptr<XdgActivationTokenData> get_token_data(Token const& token);
    std::shared_ptr<XdgActivationTokenData> create_token(struct XdgActivationTokenV1* owner, std::shared_ptr<ms::Session> session);
    void invalidate_all();
    void invalidate_if_not_from_session(std::shared_ptr<ms::Session> const&);
    auto create(std::shared_ptr<wayland_rs::Client> const& client) -> std::shared_ptr<wayland_rs::XdgActivationV1Impl> override;

private:
    class Instance : public mw::XdgActivationV1Impl
    {
    public:
        Instance(
            std::shared_ptr<wayland_rs::Client> const& client,
            mf::XdgActivationV1* xdg_activation_v1,
            std::shared_ptr<msh::Shell> const& shell,
            std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
            std::shared_ptr<MainLoop> const& main_loop);

        auto get_activation_token() -> std::shared_ptr<wayland_rs::XdgActivationTokenV1Impl> override;
        auto activate(rust::String token, wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface) -> void override;

    private:
        std::shared_ptr<wayland_rs::Client> client;
        mf::XdgActivationV1* xdg_activation_v1;
        std::shared_ptr<msh::Shell> shell;
        std::shared_ptr<ms::SessionCoordinator> const session_coordinator;
        std::shared_ptr<MainLoop> main_loop;
    };

    class KeyboardObserver: public input::KeyboardObserver
    {
    public:
        KeyboardObserver(XdgActivationV1* xdg_activation_v1);
        void keyboard_event(std::shared_ptr<MirEvent const> const& event) override;
        void keyboard_focus_set(std::shared_ptr<mi::Surface> const& surface) override;

    private:
        XdgActivationV1* xdg_activation_v1;
    };

    class SessionListener : public ms::NullSessionListener
    {
    public:
        SessionListener(XdgActivationV1* xdg_activation_v1);
        void focused(std::shared_ptr<ms::Session> const& session) override;
        void unfocused() override;

        bool is_focused_session(std::shared_ptr<ms::Session> const& other) const;

    private:
        XdgActivationV1* xdg_activation_v1;
        std::shared_ptr<ms::Session> focused_session;
    };

    void remove_token(Token const& token);

    std::shared_ptr<msh::Shell> const shell;
    std::shared_ptr<ms::SessionCoordinator> const session_coordinator;
    std::shared_ptr<MainLoop> const main_loop;
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const keyboard_observer_registrar;
    std::shared_ptr<KeyboardObserver> const keyboard_observer;
    std::shared_ptr<SessionListener> const session_listener;
    std::shared_ptr<msh::TokenAuthority> const token_authority;

    std::vector<std::shared_ptr<XdgActivationTokenData>> pending_tokens;
    std::mutex pending_tokens_mutex;
};

class XdgActivationTokenV1 : public mw::XdgActivationTokenV1Impl
{
public:
    XdgActivationTokenV1(std::shared_ptr<wayland_rs::Client> const& client, XdgActivationV1* xdg_activation);

    auto set_serial(uint32_t serial, wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat) -> void override;
    auto set_app_id(rust::String app_id) -> void override;
    auto set_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface) -> void override;
    void commit() override;

    std::optional<uint32_t> serial;
    wayland_rs::Weak<wayland_rs::WlSeatImpl> seat;

private:

    bool used{false};
    std::shared_ptr<XdgActivationTokenData> token;
    std::shared_ptr<wayland_rs::Client> client;
    XdgActivationV1* const xdg_activation;
    std::shared_ptr<ms::Session> session;
};
}
}

auto mf::create_xdg_activation_v1(
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<MainLoop> const& main_loop,
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const& keyboard_observer_registrar,
    Executor& wayland_executor,
    std::shared_ptr<msh::TokenAuthority> const& token_authority)
    -> std::shared_ptr<XdgActivationV1Global>
{
    return std::make_shared<mf::XdgActivationV1>(shell, session_coordinator, main_loop, keyboard_observer_registrar, wayland_executor, token_authority);
}

mf::XdgActivationV1::XdgActivationV1(
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<MainLoop> const& main_loop,
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const& keyboard_observer_registrar,
    Executor& wayland_executor,
    std::shared_ptr<msh::TokenAuthority> const& token_authority)
    : shell{shell},
      session_coordinator{session_coordinator},
      main_loop{main_loop},
      keyboard_observer_registrar{keyboard_observer_registrar},
      keyboard_observer{std::make_shared<KeyboardObserver>(this)},
      session_listener{std::make_shared<SessionListener>(this)},
      token_authority{token_authority}
{
    keyboard_observer_registrar->register_interest(keyboard_observer, wayland_executor);
    session_coordinator->add_listener(session_listener);
}

mf::XdgActivationV1::~XdgActivationV1()
{
    keyboard_observer_registrar->unregister_interest(*keyboard_observer);
    session_coordinator->remove_listener(session_listener);
}

std::shared_ptr<mf::XdgActivationTokenData> mf::XdgActivationV1::get_token_data(Token const& token)
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

std::shared_ptr<mf::XdgActivationTokenData> mf::XdgActivationV1::create_token(
    XdgActivationTokenV1* owner, std::shared_ptr<ms::Session> session)
{
    if (!session_listener->is_focused_session(session))
        return std::make_shared<XdgActivationTokenData>(
            token_authority->get_bogus_token(), session, owner->serial, owner->seat);

    auto generated = token_authority->issue_token(
        [this](auto const& token)
        {
            this->remove_token(token);
        });

    auto token = std::make_shared<XdgActivationTokenData>(generated, session, owner->serial, owner->seat);

    {
        std::lock_guard guard(pending_tokens_mutex);
        return pending_tokens.emplace_back(token);
    }
}

void mf::XdgActivationV1::remove_token(Token const& token)
{
    std::lock_guard guard(pending_tokens_mutex);

    std::erase_if(pending_tokens,
        [&](auto const& pending_token)
        {
            return pending_token->token == token;
        });
}

void mf::XdgActivationV1::invalidate_all()
{
    token_authority->revoke_all_tokens();
}

void mf::XdgActivationV1::invalidate_if_not_from_session(std::shared_ptr<ms::Session> const& session)
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
        token_authority->revoke_token(token->token);
}

auto mf::XdgActivationV1::create(std::shared_ptr<wayland_rs::Client> const& client) -> std::shared_ptr<wayland_rs::XdgActivationV1Impl>
{
    return std::make_shared<Instance>(client, this, shell, session_coordinator, main_loop);
}

mf::XdgActivationV1::Instance::Instance(
    std::shared_ptr<wayland_rs::Client> const& client,
    mf::XdgActivationV1* xdg_activation_v1,
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<MainLoop> const& main_loop)
    : client{client},
      xdg_activation_v1{xdg_activation_v1},
      shell{shell},
      session_coordinator{session_coordinator},
      main_loop{main_loop}
{
}

mf::XdgActivationV1::KeyboardObserver::KeyboardObserver(mir::frontend::XdgActivationV1* xdg_activation_v1)
    : xdg_activation_v1{xdg_activation_v1}
{}

void mf::XdgActivationV1::KeyboardObserver::keyboard_event(std::shared_ptr<const MirEvent> const& event)
{
    // If we encounter a key press event, then we invalidate pending activation tokens
    if (event->type() != mir_event_type_input)
        return;

    auto input_event = event->to_input();
    if (input_event->input_type() != mir_input_event_type_key)
        return;

    auto keyboard_event = input_event->to_keyboard();
    if (keyboard_event->action() == mir_keyboard_action_down)
        xdg_activation_v1->invalidate_all();
}

void mf::XdgActivationV1::KeyboardObserver::keyboard_focus_set(std::shared_ptr<mi::Surface> const&)
{
}

mf::XdgActivationV1::SessionListener::SessionListener(mf::XdgActivationV1* parent) :
    xdg_activation_v1{parent}
{
}

void mf::XdgActivationV1::SessionListener::focused(std::shared_ptr<ms::Session> const& session)
{
    focused_session = session;
    xdg_activation_v1->invalidate_if_not_from_session(session);
}

void mf::XdgActivationV1::SessionListener::unfocused()
{
    focused_session = nullptr;
    xdg_activation_v1->invalidate_all();
}

bool mf::XdgActivationV1::SessionListener::is_focused_session(std::shared_ptr<ms::Session> const& session) const
{
    return focused_session == session;
}

auto mf::XdgActivationV1::Instance::get_activation_token() -> std::shared_ptr<wayland_rs::XdgActivationTokenV1Impl>
{
    return std::make_shared<XdgActivationTokenV1>(client, this->xdg_activation_v1);
}

auto mf::XdgActivationV1::Instance::activate(rust::String string_token, wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface) -> void
{
    // This function handles requests from clients for activation.
    // This will fail if the client cannot find their token in the list.
    // A client may not be able to find their token in the list if any
    // of the following are true:
    //
    // 1. A surface other than the surface that originally requested the activation token
    //    received focus in the meantime, unless it was a layer shell surface that
    //    initially requested the focus.
    // 2. The surface failed to use the token in the alotted period of time
    // 3. A key was pressed down between the issuing of the token and the activation
    //    of the surface with that token
    //

    auto token = xdg_activation_v1->token_authority->get_token_for_string(string_token.c_str());

    // Invalid token, ignore it
    if(!token)
        return;

    // Grab a reference to the token data before removing it
    //
    // token_data can be null if the token was issued by something other
    // than `XdgActivationV1` (`ExternalClientLauncher` for example)
    auto token_data = xdg_activation_v1->get_token_data(*token);

    // Calls `remove_token` since we set that up at token creation time
    xdg_activation_v1->token_authority->revoke_token(*token);

    main_loop->enqueue(this, [
        surface=surface,
        shell=shell,
        xdg_token=token_data,
        client=client,
        session_coordinator=session_coordinator]
    {
        if (xdg_token && xdg_token->session.expired())
        {
            mir::log_error("XdgActivationV1::activate requesting session has expired");
            return;
        }

        auto const wl_surface = mf::WlSurface::from(&surface.value());
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
            auto const wl_seat = mf::WlSeatGlobal::from(&xdg_token->seat.value());
            if (!wl_seat)
                mir::log_warning("XdgActivationTokenV1::activate wl_seat not found");

            auto event = client->event_for(xdg_token->serial.value());
            if (event == std::nullopt)
                mir::log_warning("XdgActivationTokenV1::activate serial not found");
        }

        auto const& scene_surface = scene_surface_opt.value();
        auto const now = std::chrono::steady_clock::now().time_since_epoch();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds >(now).count();
        shell->raise_surface(wl_surface->session, scene_surface, ns);
    });
}

mf::XdgActivationTokenV1::XdgActivationTokenV1(
    std::shared_ptr<wayland_rs::Client> const& client, XdgActivationV1* xdg_activation) :
    client{client},
    xdg_activation{xdg_activation},
    session{client->client_session()}
{
}

auto mf::XdgActivationTokenV1::set_serial(uint32_t serial_, wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat_) -> void
{
    serial = serial_;
    seat = seat_;
}

void mf::XdgActivationTokenV1::set_app_id(rust::String app_id)
{
    // TODO: This is the application id of the surface that is coming up.
    //  Until it presents itself as a problem, we will ignore it for now.
    //  Most likely this is supposed to be used that the application who
    //  is using the application token is the application that we intend
    //  to use that token.
    (void)app_id;
}

void mf::XdgActivationTokenV1::set_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface)
{
    // TODO: This is the requesting surface. Until it presents itself
    //  as a problem, we will ignore it or now. Instead, we only ensure
    //  that the same same session is focused between token request
    //  and activation. Or we simply ensure that focus hasn't changed at all.
    (void)surface;
}

void mf::XdgActivationTokenV1::commit()
{
    if (used)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            Error::already_used,
            "The activation token has already been used"));
        return;
    }

    used = true;
    token = xdg_activation->create_token(this, session);
    send_done_event(std::string(token->token));
}
