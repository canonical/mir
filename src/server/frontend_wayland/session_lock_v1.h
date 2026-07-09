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

#ifndef MIR_FRONTEND_SESSION_LOCK_V1_H
#define MIR_FRONTEND_SESSION_LOCK_V1_H

#include "ext_session_lock_v1.h"

#include <memory>

namespace mir
{
class Executor;
namespace shell
{
class Shell;
}
namespace scene
{
class SessionLock;
}
namespace frontend
{
class WlSeat;
class OutputManager;
class SessionLockV1;
class SurfaceRegistry;

/// Tracking of the single active session lock. Shared across every bind of the
/// session-lock manager so that the "only one lock may be held" invariant is
/// global rather than per-bind.
class SessionLockState
{
public:
    explicit SessionLockState(std::shared_ptr<scene::SessionLock> session_lock);

    bool try_lock(SessionLockV1* lock);
    bool try_relinquish_locking_privilege(SessionLockV1* lock);
    bool try_unlock(SessionLockV1* lock);
    bool is_active_lock(SessionLockV1* lock) const;

private:
    std::shared_ptr<scene::SessionLock> const session_lock;
    SessionLockV1* active_lock = nullptr;
};

auto create_session_lock_manager_v1(
    std::shared_ptr<wayland_rs::Client> client,
    rust::Box<wayland_rs::ExtSessionLockManagerV1Middleware> instance,
    uint32_t object_id,
    Executor& wayland_executor,
    std::shared_ptr<shell::Shell> const& shell,
    std::shared_ptr<scene::SessionLock> const& session_lock,
    WlSeat& seat,
    OutputManager* output_manager,
    std::shared_ptr<SurfaceRegistry> const& surface_registry,
    std::shared_ptr<SessionLockState> const& state)
    -> std::shared_ptr<wayland_rs::ExtSessionLockManagerV1>;

}
}

#endif // MIR_FRONTEND_SESSION_LOCK_V1_H
