/*
 * Copyright © 2017 Canonical Ltd.
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
 */

#ifndef MIR_TOOLKIT_MIR_PERSISTENT_ID_H_
#define MIR_TOOLKIT_MIR_PERSISTENT_ID_H_

#include <mir_toolkit/client_types.h>
#include <mir_toolkit/deprecations.h>

#include <stdbool.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
/**
 * \brief Check the validity of a MirPersistentId
 * \param [in] id  The MirPersistentId
 * \return True iff the MirPersistentId contains a valid ID value.
 *
 * \note This does not guarantee that the ID refers to a currently valid object.
 */
bool mir_persistent_id_is_valid(MirPersistentId* id)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_window_id_is_valid() instead");

/**
 * \brief Free a MirPersistentId
 * \param [in] id  The MirPersistentId to free
 * \note This frees only the client-side representation; it has no effect on the
 *       object referred to by \arg id.
 */
void mir_persistent_id_release(MirPersistentId* id)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_window_id_release() instead");

/**
 * \brief Get a string representation of a MirSurfaceId
 * \param [in] id  The ID to serialise
 * \return A string representation of id. This string is owned by the MirSurfaceId,
 *         and must not be freed by the caller.
 *
 * \see mir_surface_id_from_string
 */
char const* mir_persistent_id_as_string(MirPersistentId* id)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_window_id_as_string() instead");

/**
 * \brief Deserialise a string representation of a MirSurfaceId
 * \param [in] string_representation  Serialised representation of the ID
 * \return The deserialised MirSurfaceId
 */
MirPersistentId* mir_persistent_id_from_string(char const* string_representation)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_window_id_from_string() instead");

#pragma GCC diagnostic pop

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_PERSISTENT_ID_H_ */
