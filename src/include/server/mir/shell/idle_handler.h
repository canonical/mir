/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_SHELL_IDLE_HANDLER_H_
#define MIR_SHELL_IDLE_HANDLER_H_

#include "mir/time/types.h"
#include "mir/observer_registrar.h"

#include <optional>

namespace mir
{
namespace shell
{

/// Provides a listener to the visual idle states as controlled by the IdleHandler
class IdleHandlerObserver
{
public:
    IdleHandlerObserver() = default;
    virtual ~IdleHandlerObserver() = default;
    virtual void dim() = 0;
    virtual void off() = 0;
    virtual void wake() = 0;

private:
    IdleHandlerObserver(IdleHandlerObserver const&) = delete;
    IdleHandlerObserver& operator=(IdleHandlerObserver const&) = delete;
};

class IdleHandler : public ObserverRegistrar<IdleHandlerObserver>
{
public:
    IdleHandler() = default;
    virtual ~IdleHandler() = default;

    /// Time Mir will sit idle before the display is turned off. Display may go dim some time before this. If nullopt
    /// is sent the display is never turned off or dimmed, which is the default.
    virtual void set_display_off_timeout(std::optional<time::Duration> timeout) = 0;

    /// Duration Mir will remain idle before the display is turned off after the session is locked. The display may dim
    /// some time before this. If not nullopt, this idle timeout takes precedence over the timeout set with
    /// set_display_off_timeout while the session is locked and remains idle. If Mir becomes active after the session
    /// is locked, this timeout is disabled. If the session is already locked when this function is called, the new
    /// timeout will only take effect after the next session lock.
    virtual void set_display_off_timeout_on_lock(std::optional<time::Duration> timeout) = 0;

private:
    IdleHandler(IdleHandler const&) = delete;
    IdleHandler& operator=(IdleHandler const&) = delete;
};

}
}

#endif // MIR_SHELL_IDLE_HANDLER_H_
