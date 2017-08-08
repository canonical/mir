/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
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

#ifndef MIR_CLIENT_API_HELPERS_H_
#define MIR_CLIENT_API_HELPERS_H_

#include <boost/exception/diagnostic_information.hpp> 

#include "mir/log.h"

#define MIR_LOG_UNCAUGHT_EXCEPTION(ex) { \
    mir::log_error("Caught exception at client library boundary (in %s): %s", \
                   __FUNCTION__, boost::diagnostic_information(ex).c_str()); }

#define MIR_LOG_DRIVER_BOUNDARY_EXCEPTION(ex) { \
    mir::log_error("Caught exception at Mir/EGL driver boundary (in %s): %s", \
                   __FUNCTION__, boost::diagnostic_information(ex).c_str()); }

#endif // MIR_CLIENT_API_HELPERS_H_
