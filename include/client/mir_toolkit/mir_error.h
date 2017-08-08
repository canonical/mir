/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

#include "client_types.h"

/**
 * Get the domain of a MirError.
 *
 * The error domain is required to interpret the rest of the error details.
 *
 * \param [in] error    The MirError to query
 * \returns     The MirErrorDomain that this error belongs to.
 */
MirErrorDomain mir_error_get_domain(MirError const* error);

/**
 * Get the domain-specific error code of a MirError.
 *
 * \param [in] error    The MirError to query
 * \returns     The domain-specific error code
 */
uint32_t mir_error_get_code(MirError const* error);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_ERROR_H_
