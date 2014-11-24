/*
 * Convenience functions to make logging in Mir easy
 * ~~~
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

#ifndef MIR_LOG_COMPONENT
#ifndef MIR_LOG_COMPONENT_FALLBACK
#define MIR_LOG_COMPONENT_FALLBACK "?"
#endif
#define MIR_LOG_COMPONENT MIR_LOG_COMPONENT_FALLBACK
#endif

namespace mir
{

void log_warn(std::string const& message,
              std::string const& component = MIR_LOG_COMPONENT);
void log_info(std::string const& message,
              std::string const& component = MIR_LOG_COMPONENT);

/*
 * "error" and "critical" are intentionally omitted because they're presently
 * more appropriately reported via exceptions.
 *
 * "debug" is also omitted for now, because we have yet to figure out the
 * semantics (ie. debug always logged? thresholded? include source line?)
 */

} // namespace mir

#endif // MIR_LOG_H_
