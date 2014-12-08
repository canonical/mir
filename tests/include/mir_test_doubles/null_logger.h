/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_DOUBLES_NULL_LOGGER_H_
#define MIR_TEST_DOUBLES_NULL_LOGGER_H_

#include "mir/logging/logger.h"

namespace mir
{
namespace test
{
namespace doubles
{
/// Command line option to enable logging_opt
extern char const* const logging_opt;
extern char const* const logging_descr;

class NullLogger : public mir::logging::Logger
{
void log(mir::logging::Severity, const std::string&, const std::string&) override;
};
}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_LOGGER_H_ */
