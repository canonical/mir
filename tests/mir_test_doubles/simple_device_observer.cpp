/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/test/doubles/simple_device_observer.h"

namespace mtd = mir::test::doubles;

mtd::SimpleDeviceObserver::SimpleDeviceObserver(
    std::function<void(mir::Fd&&)> on_activated,
    std::function<void()> on_suspended,
    std::function<void()> on_removed)
    : on_activated{std::move(on_activated)},
      on_suspended{std::move(on_suspended)},
      on_removed{std::move(on_removed)}
{
}

void mtd::SimpleDeviceObserver::activated(mir::Fd&& device_fd)
{
    on_activated(std::move(device_fd));
}

void mtd::SimpleDeviceObserver::suspended()
{
    on_suspended();
}

void mtd::SimpleDeviceObserver::removed()
{
    on_removed();
}
