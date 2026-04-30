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

#include "wayland_rs/wayland_rs_cpp/include/xdg_activation_v1.h"
#include "client.h"
#include <mir/observer_registrar.h>

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
}
namespace input
{
class KeyboardObserver;
}

namespace frontend
{
class XdgActivationV1Global
{
public:
    virtual ~XdgActivationV1Global() = default;
    virtual auto create( std::shared_ptr<wayland_rs::Client> const& client) -> std::shared_ptr<wayland_rs::XdgActivationV1Impl>;
};

auto create_xdg_activation_v1(
    std::shared_ptr<shell::Shell> const&,
    std::shared_ptr<scene::SessionCoordinator> const&,
    std::shared_ptr<MainLoop> const&,
    std::shared_ptr<ObserverRegistrar<input::KeyboardObserver>> const&,
    Executor& wayland_executor,
    std::shared_ptr<shell::TokenAuthority> const&) ->
    std::shared_ptr<XdgActivationV1Global>;
}
}


#endif //MIR_FRONTEND_XDG_ACTIVATION_UNSTABLE_V1_H
