/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_CLIENT_INPUT_RECEIVER_REPORT_H_
#define MIR_CLIENT_INPUT_RECEIVER_REPORT_H_

#include <mir_toolkit/event.h>

namespace mir
{
namespace input
{
namespace receiver
{

class InputReceiverReport
{
public:
    virtual ~InputReceiverReport() = default;

    virtual void received_event(MirEvent const& event) = 0;

protected:
    InputReceiverReport() = default;
    InputReceiverReport(InputReceiverReport const&) = delete;
    InputReceiverReport& operator=(InputReceiverReport const&) = delete;
};

}
}
}

#endif /* MIR_CLIENT_INPUT_RECEIVER_REPORT_H_ */
