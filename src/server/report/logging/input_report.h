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

#ifndef MIR_REPORT_LOGGING_INPUT_REPORT_H_
#define MIR_REPORT_LOGGING_INPUT_REPORT_H_

#include "mir/input/input_report.h"

#include <memory>

namespace mir
{
namespace logging
{
class Logger;
}
namespace report
{
namespace logging
{

class InputReport : public input::InputReport
{
public:
    InputReport(std::shared_ptr<mir::logging::Logger> const& logger);
    virtual ~InputReport() = default;

    void received_event_from_kernel(int64_t when, int type, int code, int value) override;

    void opened_input_device(char const* device_name, char const* input_platform) override;
    void failed_to_open_input_device(char const* device_name, char const* input_platform) override;
private:
    char const* component();
    std::shared_ptr<mir::logging::Logger> const logger;
};

}
}
}


#endif /* MIR_REPORT_LOGGING_INPUT_REPORT_H_ */
