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


#ifndef MIR_INPUT_INPUT_REPORT_H_
#define MIR_INPUT_INPUT_REPORT_H_

#include <stdint.h>

namespace mir
{
namespace input
{

class InputReport
{
public:
    virtual ~InputReport() = default;

    virtual void received_event_from_kernel(int64_t when, int type, int code, int value) = 0;

    virtual void opened_input_device(char const* device_name, char const* input_platform) = 0;
    virtual void failed_to_open_input_device(char const* device_name, char const* input_platform) = 0;

protected:
    InputReport() = default;
    InputReport(InputReport const&) = delete;
    InputReport& operator=(InputReport const&) = delete;
};

}
}

#endif /* MIR_INPUT_INPUT_REPORT_H_ */
