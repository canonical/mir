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
#include "mir/scene/surface.h"
#include "mir/shell/shell.h"
#include "mir/shell/focus_controller.h"
#include "mir/wayland/protocol_error.h"
#include "mir/log.h"
#include <random>
#include <chrono>
#include <uuid.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace msh = mir::shell;

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
class XdgActivationV1 : public wayland::XdgActivationV1::Global
{
public:
    XdgActivationV1(
        struct wl_display* display,
        std::shared_ptr<msh::Shell> const& shell,
        std::shared_ptr<msh::FocusController> const& focus_controller,
        std::shared_ptr<MainLoop> const& main_loop);

    std::string const& create_token();

    bool try_consume_token(std::string const& token);

private:
    class Instance : public wayland::XdgActivationV1
    {
    public:
        Instance(
            struct wl_resource* resource,
            mf::XdgActivationV1* xdg_activation_v1,
            std::shared_ptr<msh::Shell> const& shell,
            std::shared_ptr<msh::FocusController> const& focus_controller,
            std::shared_ptr<MainLoop> const& main_loop);

    private:
        void get_activation_token(struct wl_resource* id) override;

        void activate(std::string const& token, struct wl_resource* surface) override;

        mf::XdgActivationV1* xdg_activation_v1;
        std::shared_ptr<msh::Shell> shell;
        std::shared_ptr<msh::FocusController> const focus_controller;
        std::shared_ptr<MainLoop> main_loop;
    };

    struct Token
    {
        std::string token;
        std::unique_ptr<time::Alarm> alarm;
    };

    void bind(wl_resource* resource) override;

    std::shared_ptr<msh::Shell> shell;
    std::shared_ptr<msh::FocusController> const focus_controller;
    std::shared_ptr<MainLoop> main_loop;
    std::vector<Token> pending_tokens;
    std::mutex mutex;
};

class XdgActivationTokenV1 : public wayland::XdgActivationTokenV1
{
public:
    XdgActivationTokenV1(
        struct wl_resource* resource,
        std::string const& token,
        std::shared_ptr<msh::FocusController> const& focus_controller);

private:
    void set_serial(uint32_t serial, struct wl_resource* seat) override;

    void set_app_id(std::string const& app_id) override;

    void set_surface(struct wl_resource* surface) override;

    void commit() override;

    std::string token;
    std::shared_ptr<msh::FocusController> const focus_controller;
    std::optional<uint32_t> serial;
    std::optional<struct wl_resource*> seat;
    std::optional<std::string> app_id;
    std::optional<struct wl_resource*> requesting_surface;
    bool used = false;
};
}
}

auto mf::create_xdg_activation_v1(
    struct wl_display* display,
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<msh::FocusController> const& focus_controller,
    std::shared_ptr<MainLoop> const& main_loop)
    -> std::shared_ptr<mw::XdgActivationV1::Global>
{
    return std::make_shared<mf::XdgActivationV1>(display, shell, focus_controller, main_loop);
}

mf::XdgActivationV1::XdgActivationV1(
    struct wl_display* display,
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<msh::FocusController> const& focus_controller,
    std::shared_ptr<MainLoop> const& main_loop)
    : Global(display, Version<1>()),
      shell{shell},
      focus_controller{focus_controller},
      main_loop{main_loop}
{

}

std::string const& mf::XdgActivationV1::create_token()
{
    auto generated = generate_token();
    Token token{generated, main_loop->create_alarm([this, generated]()
    {
        try_consume_token(generated);
    })};
    token.alarm->reschedule_in(timeout_ms);

    {
        std::lock_guard guard(mutex);
        pending_tokens.emplace_back(std::move(token));
        return pending_tokens.back().token;
    }
}

bool mf::XdgActivationV1::try_consume_token(std::string const& token)
{
    {
        std::lock_guard guard(mutex);
        for (auto it = pending_tokens.begin(); it != pending_tokens.end(); it++)
        {
            if (it->token == token)
            {
                pending_tokens.erase(it);
                return true;
            }
        }
    }

    return false;
}

void mf::XdgActivationV1::bind(struct wl_resource* resource)
{
    new Instance{resource, this, shell, focus_controller, main_loop};
}

mf::XdgActivationV1::Instance::Instance(
    struct wl_resource* resource,
    mf::XdgActivationV1* xdg_activation_v1,
    std::shared_ptr<msh::Shell> const& shell,
    std::shared_ptr<msh::FocusController> const& focus_controller,
    std::shared_ptr<MainLoop> const& main_loop)
    : XdgActivationV1(resource, Version<1>()),
      xdg_activation_v1{xdg_activation_v1},
      shell{shell},
      focus_controller{focus_controller},
      main_loop{main_loop}
{
}

void mf::XdgActivationV1::Instance::get_activation_token(struct wl_resource* id)
{
    new XdgActivationTokenV1(id, xdg_activation_v1->create_token(), focus_controller);
}

void mf::XdgActivationV1::Instance::activate(std::string const& token, struct wl_resource* surface)
{
    if (!xdg_activation_v1->try_consume_token(token))
    {
        mir::log_error("XdgActivationV1::activate invalid token: %s", token.c_str());
        return;
    }

    main_loop->enqueue(this, [surface=surface, shell=shell]
    {
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

        auto const& scene_surface = scene_surface_opt.value();
        auto const now = std::chrono::steady_clock::now().time_since_epoch();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds >(now).count();
        shell->raise_surface(wl_surface->session, scene_surface, ns);
    });
}

mf::XdgActivationTokenV1::XdgActivationTokenV1(
    struct wl_resource* resource,
    std::string const& token,
    std::shared_ptr<msh::FocusController> const& focus_controller)
    : wayland::XdgActivationTokenV1(resource, Version<1>()),
      token{token},
      focus_controller{focus_controller}
{
}

void mf::XdgActivationTokenV1::set_serial(uint32_t serial_, struct wl_resource* seat_)
{
    if (serial_ != 0)
        serial = serial_;
    seat = seat_;
}

void mf::XdgActivationTokenV1::set_app_id(std::string const& app_id_)
{
    app_id = app_id_;
}

void mf::XdgActivationTokenV1::set_surface(struct wl_resource* surface)
{
    requesting_surface = surface;
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

    // In the event of a failure, we send 'done' with a new, invalid token which cannot be used later.
    if (seat && serial)
    {
        // First, assert that the seat still exists.
        auto const wl_seat = mf::WlSeat::from(seat.value());
        if (!wl_seat)
        {
            mir::log_error("XdgActivationTokenV1::commit wl_seat not found");
            send_done_event(generate_token());
            return;
        }

        auto event = client->event_for(serial.value());
        if (!event)
        {
            mir::log_error("XdgActivationTokenV1::commit serial not found");
            send_done_event(generate_token());
            return;
        }
    }

    if (app_id)
    {
        // Check that the focused application has app_id
        auto const& focused = focus_controller->focused_surface();
        if (!focused)
        {
            mir::log_error("XdgActivationTokenV1::commit cannot find the focused surface");
            send_done_event(generate_token());
            return;
        }

        if (focused->application_id() != app_id)
        {
            mir::log_error("XdgActivationTokenV1::commit app_id != focused application id");
            send_done_event(generate_token());
            return;
        }
    }

    if (requesting_surface)
    {
        // Check that the focused surface is the requesting_surface
        auto const wl_surface = mf::WlSurface::from(requesting_surface.value());
        auto const scene_surface_opt = wl_surface->scene_surface();
        if (!scene_surface_opt)
        {
            mir::log_error("XdgActivationTokenV1::commit cannot find the provided scene surface");
            send_done_event(generate_token());
            return;
        }

        auto const& scene_surface = scene_surface_opt.value();
        if (focus_controller->focused_surface() != scene_surface)
        {
            mir::log_error("XdgActivationTokenV1::commit the focused surface is not the surface that requested activation");
            send_done_event(generate_token());
            return;
        }
    }

    send_done_event(token);
}