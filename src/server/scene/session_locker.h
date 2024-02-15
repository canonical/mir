/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_SCENE_SESSION_HANDLER_H
#define MIR_SCENE_SESSION_HANDLER_H

#include "mir/frontend/session_locker.h"
#include "mir/console_services.h"
#include <memory>
#include <vector>

namespace mf = mir::frontend;

namespace mir
{
class ConsoleServices;

namespace frontend
{
class SurfaceStack;
class ScreenLockHandle;
}

namespace scene
{

class SessionLocker : public mf::SessionLocker
{
public:
    explicit SessionLocker(std::shared_ptr<mf::SurfaceStack> const&);

    void add_observer(std::shared_ptr<mf::SessionLockObserver> const&) override;
    void remove_observer(std::weak_ptr<mf::SessionLockObserver> const&) override;
    void request_lock() override;
    void request_unlock() override;

private:
    std::shared_ptr<mf::SurfaceStack> surface_stack;
    std::unique_ptr<mf::ScreenLockHandle> screen_lock_handle;
    std::vector<std::shared_ptr<mf::SessionLockObserver>> observers;

    int lock_count = 0;
};

}
}

#endif //MIR_SCENE_SESSION_HANDLER_H
