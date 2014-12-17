/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
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

#ifndef MIR_LOGGING_REQUIRE_H_
#define MIR_LOGGING_REQUIRE_H_

#include "mir/logging/logger.h"

#define MIR_REQUIRE(cond, message) { \
    mir::logging::require((cond), std::string(__FILE__) + "("+std::to_string(__LINE__) + "): " + \
    "required " + #cond + " but " + message); \
}

namespace mir
{
namespace logging
{
void require(bool required, std::string const& log_message);
}
}

#endif // MIR_LOGGING_REQUIRE_H_
