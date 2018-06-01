/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "one_shot_device_observer.h"

#include "mir/fd.h"

namespace mgc = mir::graphics::common;

mgc::OneShotDeviceObserver::OneShotDeviceObserver(mir::Fd& store_to)
    : store_to{store_to}
{
}

void mgc::OneShotDeviceObserver::activated(mir::Fd&& device_fd)
{
    store_to = std::move(device_fd);
}

void mgc::OneShotDeviceObserver::suspended()
{
}

void mgc::OneShotDeviceObserver::removed()
{
}
