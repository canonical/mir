/*
 * Copyright © 2014 Canonical Ltd.
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
 */

#ifndef MIR_VERSION_H_
#define MIR_VERSION_H_

/**
 * \addtogroup mir_toolkit
 * @{
 */


/**
 * MIR_SERVER_MAJOR_VERSION
 *
 * The major server API version. This will increase once per API incompatible release.
 *
 * See also: http://semver.org/
 */
#define MIR_SERVER_MAJOR_VERSION (0)

/**
 * MIR_SERVER_MINOR_VERSION
 *
 * The minor server API version. This will increase each time new backwards-compatible API
 * is added, and will reset to 0 each time MIR_SERVER_MAJOR_VERSION is incremented.
 * 
 * See also: http://semver.org/
 */
#define MIR_SERVER_MINOR_VERSION (10)

/**
 * MIR_SERVER_MICRO_VERSION
 *
 * The micro server API version. This will increment each release.
 * This is usually uninteresting for server code, but may be of use in selecting
 * whether to use a feature that has previously been buggy.
 *
 * This corresponds to the PATCH field of http://semver.org/
 */
#define MIR_SERVER_MICRO_VERSION (0)

/**
 * MIR_VERSION_NUMBER
 * \param major [in]    The major version (eg: 3 for version 3.2.33)
 * \param minor [in]    The minor version (eg: 2 for version 3.2.33)
 * \param micro [in]    The micro version (eg: 33 for version 3.2.33)
 * 
 * Returns the combined version information as a single 32-bit value for
 * logical comparisons. For example:
 *     #if MIR_SERVER_VERSION >= MIR_VERSION_NUMBER(2,3,4)
 *
 * This can be useful to conditionally build code depending on new features or
 * specific bugfixes in the Mir server library.
 */
#define MIR_VERSION_NUMBER(major,minor,micro) \
    (((major) << 22) + ((minor) << 12) + (micro))

/**
 * MIR_SERVER_VERSION
 *
 * The current version of the Mir server headers in use.
 */
#define MIR_SERVER_VERSION \
    MIR_VERSION_NUMBER(MIR_SERVER_MAJOR_VERSION, \
                       MIR_SERVER_MINOR_VERSION, \
                       MIR_SERVER_MICRO_VERSION)

/**@}*/

#endif /* MIR_VERSION_H_ */
