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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_LOG_H_
#define MIR_LOG_H_

#include <string>

#ifndef MIR_LOGGING_COMPONENT
#ifndef MIR_LOGGING_COMPONENT_FALLBACK
#define MIR_LOGGING_COMPONENT_FALLBACK "?"
#endif
#define MIR_LOGGING_COMPONENT MIR_LOGGING_COMPONENT_FALLBACK
#endif

namespace mir
{
void log_warn(std::string const& message,
              std::string const& component = MIR_LOGGING_COMPONENT);
void log_info(std::string const& message,
              std::string const& component = MIR_LOGGING_COMPONENT);
} // namespace mir

#endif // MIR_LOG_H_
