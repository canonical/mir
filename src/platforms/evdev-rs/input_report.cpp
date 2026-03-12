/*
 * Copyright © Canonical Ltd.
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
 */

#include "input_report.h"
#include "mir/input/input_report.h"
#include <chrono>

namespace mi = mir::input;
namespace miers = mir::input::evdev_rs;

miers::InputReport::InputReport(std::shared_ptr<mi::InputReport> const& report)
    : report(report) {}

void miers::InputReport::received_event_from_kernel(uint64_t when_microseconds, int32_t type, int32_t code, int32_t value) const
{
    report->received_event_from_kernel(
        std::chrono::microseconds(when_microseconds),
        type,
        code,
        value
    );
}
