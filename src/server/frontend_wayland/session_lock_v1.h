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

#include "ext-session-lock-v1_wrapper.h"

namespace mir
{
class Executor;
namespace shell
{
class Shell;
}
namespace frontend
{
class WlSeat;
class OutputManager;
class SessionLocker;

class SessionLockManagerV1 : public wayland::SessionLockManagerV1::Global
{
public:
    SessionLockManagerV1(
        wl_display* display,
        Executor& wayland_executor,
        std::shared_ptr<shell::Shell> shell,
        std::shared_ptr<frontend::SessionLocker> session_locker,
        WlSeat& seat,
        OutputManager* output_manager);

    Executor& wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
    std::shared_ptr<frontend::SessionLocker> const session_locker;
    WlSeat& seat;
    OutputManager* const output_manager;

private:
    class Instance;
    void bind(wl_resource* new_resource);
};

}
}

#endif // MIR_FRONTEND_SESSION_LOCK_V1_H
