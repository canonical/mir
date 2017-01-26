/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_FRAMEWORK_STUB_PLATFORM_EXTENSION_H_
#define MIR_TEST_FRAMEWORK_STUB_PLATFORM_EXTENSION_H_

#include "mir_toolkit/mir_extension_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef char const* (*mir_extension_favorite_flavor)();
typedef struct MirExtensionFavoriteFlavorV1
{
    mir_extension_favorite_flavor favorite_flavor;
} MirExtensionFavoriteFlavor;
typedef MirExtensionFavoriteFlavorV1 MirExtensionFavoriteFlavorV9;

typedef char const* (*mir_extension_animal_name)();
typedef struct MirExtensionAnimalNamesV1
{
    mir_extension_animal_name animal_name;
} MirExtensionAnimalNames;

static inline MirExtensionFavoriteFlavorV1 const* mir_extension_favorite_flavor_v1(
    MirConnection* connection)
{
    return (MirExtensionFavoriteFlavorV1 const*) mir_connection_request_extension(
        connection, "mir_extension_favorite_flavor", 1);
}

static inline MirExtensionFavoriteFlavorV9 const* mir_extension_favorite_flavor_v9(
    MirConnection* connection)
{
    return (MirExtensionFavoriteFlavorV9 const*) mir_connection_request_extension(
        connection, "mir_extension_favorite_flavor", 9);
}

static inline MirExtensionAnimalNamesV1 const* mir_extension_animal_names_v1(
    MirConnection* connection)
{
    return (MirExtensionAnimalNamesV1 const*) mir_connection_request_extension(
        connection, "mir_extension_animal_names", 1); 
}

#ifdef __cplusplus
}
#endif
#endif /* MIR_TEST_FRAMEWORK_STUB_PLATFORM_EXTENSION_H_ */
