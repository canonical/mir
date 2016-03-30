/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_TOOLKIT_MIR_ERROR_H_
#define MIR_TOOLKIT_MIR_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "client_types.h"

char const* mir_error_get_debug_message(MirError const* error);

char const* mir_error_get_display_message(MirError const* error);

MirErrorDomain mir_error_get_domain(MirError const* error);

#ifdef __cplusplus
}
#endif

#endif // MIR_TOOLKIT_MIR_ERROR_H_
