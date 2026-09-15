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

#include "xdg_activation_v1.h"
#include <mir/observer_registrar.h>
#include <mir/shell/token_authority.h>

#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace mir
{
class MainLoop;
class Executor;
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
struct XdgActivationTokenData
{
    XdgActivationTokenData(
        shell::TokenAuthority::Token const& token,
        std::shared_ptr<scene::Session> session,
        std::optional<uint32_t> serial,
        std::optional<wayland::Weak<wayland::Seat>> seat) :
        token{token},
        session{session},
        serial{serial},
        seat{seat}
    {
    }

    shell::TokenAuthority::Token const token;
    std::weak_ptr<scene::Session> session;
    std::optional<uint32_t> serial;
    std::optional<wayland::Weak<wayland::Seat>> seat;
};

/// The cross-client shared state previously held by the `XdgActivationV1` global.
/// Owned once by the connector/factory; every per-bind `wayland::XdgActivationV1`
/// object refers to a single instance of this.
class XdgActivationV1State
{
public:
    using Token = shell::TokenAuthority::Token;

    XdgActivationV1State(
        std::shared_ptr<shell::Shell> const& shell,
        std::shared_ptr<scene::SessionCoordinator> const& session_coordinator,
        std::shared_ptr<MainLoop> const& main_loop,
        std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const& keyboard_observer_registrar,
        Executor& wayland_executor,
        std::shared_ptr<shell::TokenAuthority> const& token_authority);
    ~XdgActivationV1State();

    auto shell() const -> std::shared_ptr<shell::Shell> const& { return shell_; }
    auto session_coordinator() const -> std::shared_ptr<scene::SessionCoordinator> const& { return session_coordinator_; }
    auto main_loop() const -> std::shared_ptr<MainLoop> const& { return main_loop_; }
    auto token_authority() const -> std::shared_ptr<shell::TokenAuthority> const& { return token_authority_; }

    auto get_token_data(Token const& token) -> std::shared_ptr<XdgActivationTokenData>;
    auto create_token(
        std::optional<uint32_t> serial,
        std::optional<wayland::Weak<wayland::Seat>> seat,
        std::shared_ptr<scene::Session> session) -> std::shared_ptr<XdgActivationTokenData>;
    void invalidate_all();
    void invalidate_if_not_from_session(std::shared_ptr<scene::Session> const&);

private:
    class KeyboardObserver;
    class SessionListener;

    void remove_token(Token const& token);

    std::shared_ptr<shell::Shell> const shell_;
    std::shared_ptr<scene::SessionCoordinator> const session_coordinator_;
    std::shared_ptr<MainLoop> const main_loop_;
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const keyboard_observer_registrar;
    std::shared_ptr<KeyboardObserver> const keyboard_observer;
    std::shared_ptr<SessionListener> const session_listener;
    std::shared_ptr<shell::TokenAuthority> const token_authority_;

    std::vector<std::shared_ptr<XdgActivationTokenData>> pending_tokens;
    std::mutex pending_tokens_mutex;
};

auto create_xdg_activation_v1(
    std::shared_ptr<wayland::Client> client,
    rust::Box<wayland::XdgActivationV1Middleware> instance,
    uint32_t object_id,
    XdgActivationV1State& state) -> std::shared_ptr<wayland::XdgActivationV1>;
}
}


#endif //MIR_FRONTEND_XDG_ACTIVATION_UNSTABLE_V1_H
