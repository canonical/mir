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
#include "mir/observer_multiplexer.h"
#include <memory>

namespace mf = mir::frontend;

namespace mir
{
class Executor;
class ConsoleServices;
class MainLoop;

namespace frontend
{
class SurfaceStack;
class ScreenLockHandle;
}

namespace scene
{

class SessionLockerObserverMultiplexer : public ObserverMultiplexer<mf::SessionLockObserver>
{
public:
    explicit SessionLockerObserverMultiplexer(std::shared_ptr<Executor> const& executor);
    void on_lock() override;
    void on_unlock() override;
};

class SessionLocker : public mf::SessionLocker
{
public:
    explicit SessionLocker(
        std::shared_ptr<Executor> const&,
        std::shared_ptr<mf::SurfaceStack> const&);

    void lock() override;
    void unlock() override;

    void register_interest(std::weak_ptr<mf::SessionLockObserver> const& observer) override;
    void register_interest(
        std::weak_ptr<mf::SessionLockObserver> const& observer,
        Executor& executor) override;
    void unregister_interest(mf::SessionLockObserver const& observer) override;

private:
    std::shared_ptr<Executor> executor;
    std::shared_ptr<mf::SurfaceStack> surface_stack;
    std::unique_ptr<mf::ScreenLockHandle> screen_lock_handle;
    SessionLockerObserverMultiplexer multiplexer;
};

}
}

#endif //MIR_SCENE_SESSION_HANDLER_H
