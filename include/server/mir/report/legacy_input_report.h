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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_REPORT_LEGACY_INPUT_REPORT_H_
#define MIR_REPORT_LEGACY_INPUT_REPORT_H_

#include <memory>

namespace mir
{
namespace logging
{
class Logger;
}
namespace report
{
namespace legacy_input
{
void initialize(std::shared_ptr<mir::logging::Logger> const& logger);
}
}
}

#endif // MIR_REPORT_LEGACY_INPUT_REPORT_H_
