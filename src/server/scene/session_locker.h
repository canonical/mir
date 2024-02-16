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
#include <memory>

namespace mf = mir::frontend;

namespace mir
{
class ConsoleServices;
class MainLoop;

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
    explicit SessionLocker(
        std::shared_ptr<Executor> const&,
        std::shared_ptr<mf::SurfaceStack> const&);

    void on_lock() override;
    void on_unlock() override;

private:
    std::shared_ptr<Executor> executor;
    std::shared_ptr<mf::SurfaceStack> surface_stack;
    std::unique_ptr<mf::ScreenLockHandle> screen_lock_handle;
};

}
}

#endif //MIR_SCENE_SESSION_HANDLER_H
