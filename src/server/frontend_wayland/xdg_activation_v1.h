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

#ifndef MIR_FRONTEND_XDG_ACTIVATION_UNSTABLE_V1_H
#define MIR_FRONTEND_XDG_ACTIVATION_UNSTABLE_V1_H

#include "mir/input/keyboard_observer.h"
#include "mir/scene/null_session_listener.h"
#include "mir/scene/session_listener.h"
#include "mir/server.h"
#include "mir/shell/token_authority.h"
#include "xdg-activation-v1_wrapper.h"
#include "mir/observer_registrar.h"

struct wl_display;

namespace mir
{
class MainLoop;
namespace shell
{
class Shell;
class TokenAuthority;
}
namespace scene
{
class SessionCoordinator;
class Session;
}
namespace input
{
class KeyboardObserver;
}

namespace frontend
{
auto create_xdg_activation_v1(
    struct wl_display* display,
    std::shared_ptr<shell::Shell> const&,
    std::shared_ptr<scene::SessionCoordinator> const&,
    std::shared_ptr<MainLoop> const&,
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const&,
    Executor& wayland_executor,
    std::shared_ptr<shell::TokenAuthority> const&) ->
    std::shared_ptr<wayland::XdgActivationV1::Global>;

class XdgActivationTokenData;
class BufferStream;
class XdgActivationV1 : public wayland::XdgActivationV1::Global
{
public:
    XdgActivationV1(
        struct wl_display* display,
        std::shared_ptr<mir::shell::Shell> const& shell,
        std::shared_ptr<mir::scene::SessionCoordinator> const& session_coordinator,
        std::shared_ptr<MainLoop> const& main_loop,
        std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const& keyboard_observer_registrar,
        Executor& wayland_executor,
        std::shared_ptr<shell::TokenAuthority> token_authority);
    ~XdgActivationV1();

    std::shared_ptr<XdgActivationTokenData> get_token_data(std::string const& token);
    std::shared_ptr<XdgActivationTokenData> const& create_token(std::shared_ptr<mir::scene::Session> const& session);
    void invalidate_all();
    void invalidate_if_not_from_session(std::shared_ptr<mir::scene::Session> const&);

private:
    class Instance : public wayland::XdgActivationV1
    {
    public:
        Instance(
            struct wl_resource* resource,
            mir::frontend::XdgActivationV1* xdg_activation_v1,
            std::shared_ptr<mir::shell::Shell> const& shell,
            std::shared_ptr<mir::scene::SessionCoordinator> const& session_coordinator,
            std::shared_ptr<MainLoop> const& main_loop);

    private:
        void get_activation_token(struct wl_resource* id) override;

        void activate(std::string const& token, struct wl_resource* surface) override;

        mir::frontend::XdgActivationV1* xdg_activation_v1;
        std::shared_ptr<mir::shell::Shell> shell;
        std::shared_ptr<mir::scene::SessionCoordinator> const session_coordinator;
        std::shared_ptr<MainLoop> main_loop;
    };

    class KeyboardObserver: public input::KeyboardObserver
    {
    public:
        KeyboardObserver(mir::frontend::XdgActivationV1* xdg_activation_v1);
        void keyboard_event(std::shared_ptr<MirEvent const> const& event) override;
        void keyboard_focus_set(std::shared_ptr<mir::input::Surface> const& surface) override;

    private:
        mir::frontend::XdgActivationV1* xdg_activation_v1;
    };

    class SessionListener : public mir::scene::NullSessionListener
    {
    public:
        SessionListener(mir::frontend::XdgActivationV1* xdg_activation_v1);
        void focused(std::shared_ptr<mir::scene::Session> const& session) override;

    private:
        mir::frontend::XdgActivationV1* xdg_activation_v1;
    };

    void bind(wl_resource* resource) override;
    auto find_token_unlocked(std::string const& token) const;
    void remove_token(std::string const& token);


    std::shared_ptr<mir::shell::Shell> shell;
    std::shared_ptr<mir::scene::SessionCoordinator> const session_coordinator;
    std::shared_ptr<MainLoop> main_loop;
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> keyboard_observer_registrar;
    std::shared_ptr<KeyboardObserver> keyboard_observer;
    std::shared_ptr<SessionListener> session_listener;
    std::vector<std::shared_ptr<XdgActivationTokenData>> pending_tokens;
    std::shared_ptr<shell::TokenAuthority> token_authority;
    std::mutex pending_tokens_mutex;
};
}
}


#endif //MIR_FRONTEND_XDG_ACTIVATION_UNSTABLE_V1_H
