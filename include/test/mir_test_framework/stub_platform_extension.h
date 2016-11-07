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

#define MIR_EXTENSION_FAVORITE_FLAVOR "665f8be5-996f-4557-8404-f1c7ed94c04a"
#define MIR_EXTENSION_ANIMAL_NAME    "817e4327-bdd7-495a-9d3c-b5ac7a8a831f"

#define MIR_EXTENSION_FAVORITE_FLAVOR_VERSION_1 1
#define MIR_EXTENSION_FAVORITE_FLAVOR_VERSION_2 2
#define MIR_EXTENSION_ANIMAL_NAME_VERSION_9 9

#ifdef __cplusplus
extern "C" {
#endif

typedef char const* (*mir_extension_favorite_flavor)();
struct MirExtensionFavoriteFlavor
{
    mir_extension_favorite_flavor favorite_flavor;
};

typedef char const* (*mir_extension_animal_name)();
struct MirExtensionAnimalNames 
{
    mir_extension_animal_name animal_name;
};

#ifdef __cplusplus
}
#endif
#endif /* MIR_TEST_FRAMEWORK_STUB_PLATFORM_EXTENSION_H_ */
