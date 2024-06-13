/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_REPORT_LTTNG_INPUT_REPORT_H_
#define MIR_REPORT_LTTNG_INPUT_REPORT_H_

#include "server_tracepoint_provider.h"

#include "mir/input/input_report.h"

namespace mir
{
namespace report
{
namespace lttng
{

class InputReport : public input::InputReport
{
public:
    InputReport() = default;
    virtual ~InputReport() = default;

    void received_event_from_kernel(int64_t when, int type, int code, int value) override;

    void opened_input_device(char const* device_name, char const* input_platform) override;
    void failed_to_open_input_device(char const* device_name, char const* input_platform) override;
private:
    ServerTracepointProvider tp_provider;
};

}
}
}


#endif /* MIR_REPORT_LTTNG_INPUT_REPORT_H_ */
