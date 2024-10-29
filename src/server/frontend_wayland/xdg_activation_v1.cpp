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
#include "wl_surface.h"
#include "wl_seat.h"
#include "wl_client.h"
#include "mir/main_loop.h"
#include "mir/input/keyboard_observer.h"
#include "mir/scene/surface.h"
#include "mir/scene/session_listener.h"
#include "mir/scene/session_coordinator.h"
#include "mir/shell/shell.h"
#include "mir/wayland/protocol_error.h"
#include "mir/log.h"
#include <random>
#include <chrono>
#include <uuid.h>
#include <mir/events/keyboard_event.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mi = mir::input;

namespace
{
/// Time in milliseconds that the compositor will wait before invalidating a token
auto constexpr timeout_ms = std::chrono::milliseconds(3000);

std::string generate_token()
{
    uuid_t uuid;
    uuid_generate(uuid);
    return { reinterpret_cast<char*>(uuid), 4 };
}
}

namespace mir
{
namespace frontend
{
struct XdgActivationTokenData
{
    XdgActivationTokenData(
        std::string const& token,
        std::unique_ptr<time::Alarm> alarm_,
        std::shared_ptr<ms::Session> const& session)
        : token{token},
        alarm{std::move(alarm_)},
        session{session}
    {
        alarm->reschedule_in(timeout_ms);
    }

    std::string const token;
    std::unique_ptr<time::Alarm> const alarm;
    std::weak_ptr<ms::Session> session;

    std::optional<uint32_t> serial;
    std::optional<struct wl_resource*> seat;
};


class XdgActivationV1 : public wayland::XdgActivationV1::Global
{
public:
    XdgActivationV1(
        struct wl_display* display,
        std::shared_ptr<msh::Shell> const& shell,
        std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
        std::shared_ptr<MainLoop> const& main_loop,
        std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const& keyboard_observer_registrar,
        Executor& wayland_executor);
    ~XdgActivationV1();

    std::shared_ptr<XdgActivationTokenData> const& create_token(std::shared_ptr<ms::Session> const& session);
    std::shared_ptr<XdgActivationTokenData> try_consume_token(std::string const& token);
    void invalidate_all();
    void invalidate_if_not_from_session(std::shared_ptr<ms::Session> const&);

private:
    class Instance : public wayland::XdgActivationV1
    {
    public:
        Instance(
            struct wl_resource* resource,
            mf::XdgActivationV1* xdg_activation_v1,
            std::shared_ptr<msh::Shell> const& shell,
            std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
            std::shared_ptr<MainLoop> const& main_loop);

    private:
        void get_activation_token(struct wl_resource* id) override;

        void activate(std::string const& token, struct wl_resource* surface) override;

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

    class SessionListener : public ms::SessionListener
    {
    public:
        SessionListener(XdgActivationV1* xdg_activation_v1);
        void starting(std::shared_ptr<ms::Session> const&) override {}
        void stopping(std::shared_ptr<ms::Session> const&) override {}
        void focused(std::shared_ptr<ms::Session> const& session) override;
        void unfocused() override {}

        void surface_created(ms::Session&, std::shared_ptr<ms::Surface> const&) override {}
        void destroying_surface(ms::Session&, std::shared_ptr<ms::Surface> const&) override {}

        void buffer_stream_created(
            ms::Session&,
            std::shared_ptr<mf::BufferStream> const&) override {}
        void buffer_stream_destroyed(
            ms::Session&,
            std::shared_ptr<mf::BufferStream> const&) override {}

    private:
        XdgActivationV1* xdg_activation_v1;
    };

    void bind(wl_resource* resource) override;

    std::shared_ptr<msh::Shell> shell;
    std::shared_ptr<ms::SessionCoordinator> const session_coordinator;
    std::shared_ptr<MainLoop> main_loop;
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> keyboard_observer_registrar;
    std::shared_ptr<KeyboardObserver> keyboard_observer;
    std::vector<std::shared_ptr<XdgActivationTokenData>> pending_tokens;
    std::mutex pending_tokens_mutex;
};

class XdgActivationTokenV1 : public wayland::XdgActivationTokenV1
{
public:
    XdgActivationTokenV1(
        struct wl_resource* resource,
        std::shared_ptr<XdgActivationTokenData> const& token);

private:
    void set_serial(uint32_t serial, struct wl_resource* seat) override;

    void set_app_id(std::string const& app_id) override;

    void set_surface(struct wl_resource* surface) override;

    void commit() override;

    std::shared_ptr<XdgActivationTokenData> token;
    bool used = false;
};
}
}

auto mf::create_xdg_activation_v1(
    struct wl_display* display,
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<MainLoop> const& main_loop,
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const& keyboard_observer_registrar,
    Executor& wayland_executor)
    -> std::shared_ptr<mw::XdgActivationV1::Global>
{
    return std::make_shared<mf::XdgActivationV1>(display, shell, session_coordinator, main_loop, keyboard_observer_registrar, wayland_executor);
}

mf::XdgActivationV1::XdgActivationV1(
    struct wl_display* display,
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<MainLoop> const& main_loop,
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const& keyboard_observer_registrar,
    Executor& wayland_executor)
    : Global(display, Version<1>()),
      shell{shell},
      session_coordinator{session_coordinator},
      main_loop{main_loop},
      keyboard_observer_registrar{keyboard_observer_registrar},
      keyboard_observer{std::make_shared<KeyboardObserver>(this)}
{
    keyboard_observer_registrar->register_interest(keyboard_observer, wayland_executor);
}

mf::XdgActivationV1::~XdgActivationV1()
{
    keyboard_observer_registrar->unregister_interest(*keyboard_observer);
}

std::shared_ptr<mf::XdgActivationTokenData> const& mf::XdgActivationV1::create_token(std::shared_ptr<ms::Session> const& session)
{
    auto generated = generate_token();
    auto token = std::make_shared<XdgActivationTokenData>(generated, main_loop->create_alarm([this, generated]()
    {
        try_consume_token(generated);
    }), session);

    {
        std::lock_guard guard(pending_tokens_mutex);
        pending_tokens.emplace_back(std::move(token));
        return pending_tokens.back();
    }
}

std::shared_ptr<mf::XdgActivationTokenData> mf::XdgActivationV1::try_consume_token(std::string const& token)
{
    {
        std::lock_guard guard(pending_tokens_mutex);
        for (auto it = pending_tokens.begin(); it != pending_tokens.end(); it++)
        {
            if (it->get()->token == token)
            {
                auto result = *it;
                pending_tokens.erase(it);
                return result;
            }
        }
    }

    return nullptr;
}

void mf::XdgActivationV1::invalidate_all()
{
    std::lock_guard guard(pending_tokens_mutex);
    pending_tokens.clear();
}

void mf::XdgActivationV1::invalidate_if_not_from_session(std::shared_ptr<ms::Session> const& session)
{
    std::lock_guard guard(pending_tokens_mutex);
    std::erase_if(pending_tokens, [&](std::shared_ptr<XdgActivationTokenData> const& token)
    {
        return token->session.expired() || token->session.lock() != session;
    });
}

void mf::XdgActivationV1::bind(struct wl_resource* resource)
{
    new Instance{resource, this, shell, session_coordinator, main_loop};
}

mf::XdgActivationV1::Instance::Instance(
    struct wl_resource* resource,
    mf::XdgActivationV1* xdg_activation_v1,
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<MainLoop> const& main_loop)
    : XdgActivationV1(resource, Version<1>()),
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

void mf::XdgActivationV1::SessionListener::focused(std::shared_ptr<ms::Session> const& session)
{
    xdg_activation_v1->invalidate_if_not_from_session(session);
}

void mf::XdgActivationV1::Instance::get_activation_token(struct wl_resource* id)
{
    new XdgActivationTokenV1(id, xdg_activation_v1->create_token(client->client_session()));
}

void mf::XdgActivationV1::Instance::activate(std::string const& token, struct wl_resource* surface)
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
    auto xdg_token = xdg_activation_v1->try_consume_token(token);
    if (!xdg_token)
    {
        mir::log_error("XdgActivationV1::activate invalid token: %s", token.c_str());
        return;
    }

    main_loop->enqueue(this, [
        surface=surface,
        shell=shell,
        xdg_token=xdg_token,
        client=client,
        session_coordinator=session_coordinator]
    {
        if (xdg_token->session.expired())
        {
            mir::log_error("XdgActivationV1::activate requesting session has expired");
            return;
        }

        auto const wl_surface = mf::WlSurface::from(surface);
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
        if (xdg_token->seat && xdg_token->serial)
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
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds >(now).count();
        shell->raise_surface(wl_surface->session, scene_surface, ns);
    });
}

mf::XdgActivationTokenV1::XdgActivationTokenV1(
    struct wl_resource* resource,
    std::shared_ptr<XdgActivationTokenData> const& token)
    : wayland::XdgActivationTokenV1(resource, Version<1>()),
      token{token}
{
}

void mf::XdgActivationTokenV1::set_serial(uint32_t serial_, struct wl_resource* seat_)
{
    token->serial = serial_;
    token->seat = seat_;
}

void mf::XdgActivationTokenV1::set_app_id(std::string const& app_id)
{
    // TODO: This is the application id of the surface that is coming up.
    //  Until it presents itself as a problem, we will ignore it for now.
    //  Most likely this is supposed to be used that the application who
    //  is using the application token is the application that we intend
    //  to use that token.
    (void)app_id;
}

void mf::XdgActivationTokenV1::set_surface(struct wl_resource* surface)
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
            resource,
            Error::already_used,
            "The activation token has already been used"));
        return;
    }

    used = true;
    send_done_event(token->token);
}
