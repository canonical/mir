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

#include "frame_executor.h"

#include <mir/main_loop.h>

#include <mutex>
#include <vector>

namespace mf = mir::frontend;

namespace
{
auto const delay = std::chrono::milliseconds{16};
}

struct mf::FrameExecutor::Callbacks
{
    std::mutex mutex;
    std::vector<std::function<void()>> queued;
};

mf::FrameExecutor::FrameExecutor(time::AlarmFactory& alarm_factory)
    : callbacks{std::make_shared<Callbacks>()},
      alarm{alarm_factory.create_alarm([weak_callbacks = std::weak_ptr<Callbacks>{callbacks}]()
          {
              fire_callbacks(weak_callbacks);
          })}
{
}

mf::FrameExecutor::~FrameExecutor() = default;

void mf::FrameExecutor::spawn(std::function<void()>&& work)
{
    std::unique_lock lock{callbacks->mutex};
    bool const needs_alarm = callbacks->queued.empty();
    callbacks->queued.push_back(std::move(work));
    lock.unlock();

    if (needs_alarm)
    {
        alarm->reschedule_in(delay);
    }
}

void mf::FrameExecutor::fire_callbacks(std::weak_ptr<Callbacks> const& weak_callbacks)
{
    if (auto const callbacks = weak_callbacks.lock())
    {
        std::unique_lock lock{callbacks->mutex};
        auto const queued = std::move(callbacks->queued);
        callbacks->queued.clear();
        lock.unlock();

        for (auto const& callback : queued)
        {
            callback();
        }
    }
}
