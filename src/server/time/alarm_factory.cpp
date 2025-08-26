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

#include "mir/time/alarm_factory.h"

auto mir::time::AlarmFactory::create_repeating_alarm(
    std::function<void()> const& callback, std::chrono::milliseconds repeat_delay) -> std::shared_ptr<Alarm>
{
    auto const shared_weak_alarm{std::make_shared<std::weak_ptr<time::Alarm>>()};

    std::shared_ptr result = create_alarm(
        [swk = shared_weak_alarm, cb = std::move(callback), repeat_delay]()
        {
            cb();

            if (auto const& repeat_alarm = swk->lock())
                repeat_alarm->reschedule_in(repeat_delay);
        });

    *shared_weak_alarm = result;

    return result;
}
