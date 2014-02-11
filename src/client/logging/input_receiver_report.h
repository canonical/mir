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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_CLIENT_LOGGING_INPUT_RECEIVER_REPORT_H_
#define MIR_CLIENT_LOGGING_INPUT_RECEIVER_REPORT_H_

#include "mir/input/input_receiver_report.h"

#include <memory>

namespace mir
{

namespace logging
{
class Logger;
}

namespace client
{
namespace logging
{

class InputReceiverReport : public input::receiver::InputReceiverReport
{
public:
    InputReceiverReport(std::shared_ptr<mir::logging::Logger> const& logger);

    void received_event(MirEvent const& event) override;

private:
    std::shared_ptr<mir::logging::Logger> const logger;
};

}
}
}

#endif /* MIR_CLIENT_LOGGING_INPUT_RECEIVER_REPORT_H_ */
