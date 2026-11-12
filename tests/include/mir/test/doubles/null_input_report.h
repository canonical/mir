/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_DOUBLES_NULL_INPUT_REPORT_H_
#define MIR_TEST_DOUBLES_NULL_INPUT_REPORT_H_

#include <mir/input/input_report.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct NullInputReport : mir::input::InputReport
{
    void received_event_from_kernel(std::chrono::nanoseconds, int, int, int) override {}
};
}
}
}

#endif //MIR_TEST_DOUBLES_NULL_INPUT_REPORT_H_
