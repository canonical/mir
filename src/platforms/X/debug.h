/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_GRAPHICS_X_DEBUG_H_
#define MIR_GRAPHICS_X_DEBUG_H_

#define MIR_LOG_COMPONENT "X11-platform"
#include "mir/log.h"

// Set DEBUG=1 for debug info, 0 for silent operation
#define DEBUG 1

#if DEBUG
#define CALLED \
{ \
    mir::log_info("%s", __PRETTY_FUNCTION__); \
}
#else
#define CALLED
#endif

#endif /* MIR_GRAPHICS_X_DEBUG_H_ */
