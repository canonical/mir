/*
 * Copyright © 2021 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_FRAME_CALLBACK_EXECUTOR_H
#define MIR_FRONTEND_FRAME_CALLBACK_EXECUTOR_H

#include <mir/executor.h>

#include <vector>
#include <mutex>
#include <memory>

namespace mir
{
namespace time
{
class Alarm;
class AlarmFactory;
}

namespace frontend
{

/// Runs frame callbacks that do not have a buffer to be attached to. Is threadsafe. Given callbacks are run on the
/// main loop thread, the wayland executor is NOT automatically used.
class FrameCallbackExecutor : public Executor
{
public:
    FrameCallbackExecutor(time::AlarmFactory& alarm_factory);

    void spawn(std::function<void()>&& work) override;

private:
    struct Callbacks
    {
        std::mutex mutex;
        std::vector<std::function<void()>> queued;
    };

    std::shared_ptr<Callbacks> const callbacks; // shared_ptr so it can potentially outlive this object
    std::unique_ptr<time::Alarm> const alarm;

    static void fire_callbacks(std::weak_ptr<Callbacks> const& weak_callbacks);
};

}
}

#endif // MIR_FRONTEND_FRAME_CALLBACK_EXECUTOR_H
