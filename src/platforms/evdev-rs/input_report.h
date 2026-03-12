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

#ifndef MIR_INPUT_EVDEV_RS_INPUT_REPORT_H
#define MIR_INPUT_EVDEV_RS_INPUT_REPORT_H

#include <memory>
#include <cstdint>

namespace mir
{
namespace input
{
class InputReport;

namespace evdev_rs
{
class InputReport
{
public:
    explicit InputReport(std::shared_ptr<input::InputReport> const& report);
    void received_event_from_kernel(uint64_t when_microseconds, int32_t type, uint32_t code, uint32_t value) const;

private:
    std::shared_ptr<input::InputReport> report;
};
}
}
}

#endif
