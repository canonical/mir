/*
 * Copyright © 2021 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */

#include "basic_idle_hub.h"
#include "mir/time/alarm.h"
#include "mir/time/alarm_factory.h"

namespace ms = mir::scene;
namespace mt = mir::time;

namespace
{
auto add_awake_at_start(
    std::vector<ms::BasicIdleHub::StateEntry> const& state_sequence)-> std::vector<ms::BasicIdleHub::StateEntry>
{
    std::vector<ms::BasicIdleHub::StateEntry> result{
        {std::chrono::milliseconds{0}, ms::IdleState::awake}};
    result.insert(result.end(), state_sequence.begin(), state_sequence.end());
    return result;
}
}

ms::BasicIdleHub::BasicIdleHub(
        std::vector<StateEntry> const& state_sequence,
        std::shared_ptr<mt::AlarmFactory> const& alarm_factory)
    : state_sequence{add_awake_at_start(state_sequence)},
      next_state_alarm{alarm_factory->create_alarm([this]()
          {
              increment_state();
          })},
      current_state_index{0}
{
    std::unique_lock<std::mutex> lock{mutex};
    set_state_index(lock, 0);
}

ms::BasicIdleHub::~BasicIdleHub()
{
}

auto ms::BasicIdleHub::state() -> IdleState
{
    std::lock_guard<std::mutex> lock{mutex};
    return state_sequence[current_state_index].state;
}

void ms::BasicIdleHub::poke()
{
    std::unique_lock<std::mutex> lock{mutex};
    set_state_index(lock, 0);
}

void ms::BasicIdleHub::increment_state()
{
    std::unique_lock<std::mutex> lock{mutex};
    if (current_state_index < static_cast<int>(state_sequence.size()))
    {
        set_state_index(lock, current_state_index + 1);
    }
}

void ms::BasicIdleHub::set_state_index(std::unique_lock<std::mutex>& lock, int index)
{
    if (index + 1 < static_cast<int>(state_sequence.size()))
    {
        next_state_alarm->reschedule_in(state_sequence[index + 1].time_from_previous);
    }
    else
    {
        next_state_alarm->cancel();
    }
    auto const prev_state = state_sequence[current_state_index].state;
    auto const new_state = state_sequence[index].state;
    current_state_index = index;
    lock.unlock();
    if (prev_state != new_state)
    {
        multiplexer.idle_state_changed(new_state);
    }
}

void ms::BasicIdleHub::send_initial_state(std::weak_ptr<IdleStateObserver> const& observer)
{
    if (auto const shared = observer.lock())
    {
        shared->idle_state_changed(state());
    }
}
