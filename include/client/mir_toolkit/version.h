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
 */

#ifndef MIR_TOOLKIT_VERSION_H_
#define MIR_TOOLKIT_VERSION_H_

#include "mir_toolkit/mir_version_number.h"

/**
 * \addtogroup mir_toolkit
 * @{
 */


/**
 * MIR_CLIENT_MAJOR_VERSION
 *
 * The major client API version. This will increase once per API incompatible release.
 *
 * See also: http://semver.org/
 */
#define MIR_CLIENT_MAJOR_VERSION (3)

/**
 * MIR_CLIENT_MINOR_VERSION
 *
 * The minor client API version. This will increase each time new backwards-compatible API
 * is added, and will reset to 0 each time MIR_CLIENT_MAJOR_VERSION is incremented.
 * 
 * See also: http://semver.org/
 */
#define MIR_CLIENT_MINOR_VERSION (3)

/**
 * MIR_CLIENT_MICRO_VERSION
 *
 * The micro client API version. This will increment each release.
 * This is usually uninteresting for client code, but may be of use in selecting
 * whether to use a feature that has previously been buggy.
 *
 * This corresponds to the PATCH field of http://semver.org/
 */
#define MIR_CLIENT_MICRO_VERSION (0)

/**
 * MIR_CLIENT_VERSION
 *
 * The current version of the Mir client headers in use.
 */
#define MIR_CLIENT_VERSION \
    MIR_VERSION_NUMBER(MIR_CLIENT_MAJOR_VERSION, \
                       MIR_CLIENT_MINOR_VERSION, \
                       MIR_CLIENT_MICRO_VERSION)

/**@}*/

#endif /* MIR_TOOLKIT_VERSION_H_ */
