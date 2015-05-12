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

#include "input_receiver_report.h"

#include "mir/logging/logger.h"
#include "mir/logging/input_timestamp.h"
#include "mir/event_printer.h"

#include <boost/throw_exception.hpp>

#include <sstream>
#include <stdexcept>

namespace ml = mir::logging;
namespace mcll = mir::client::logging;

namespace
{
std::string const component{"input-receiver"};
}

mcll::InputReceiverReport::InputReceiverReport(std::shared_ptr<ml::Logger> const& logger)
    : logger{logger}
{
}

void mcll::InputReceiverReport::received_event(
    MirEvent const& event)
{
    std::stringstream ss;

    ss << "Received event:" << event << std::endl;

    logger->log(ml::Severity::debug, ss.str(), component);
}
