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

#ifndef MIR_VERSION_NUMBER_H_
#define MIR_VERSION_NUMBER_H_

/**
 * MIR_VERSION_NUMBER
 * \param major [in]    The major version (eg: 3 for version 3.2.33)
 * \param minor [in]    The minor version (eg: 2 for version 3.2.33)
 * \param micro [in]    The micro version (eg: 33 for version 3.2.33)
 *
 * Returns the combined version information as a single 32-bit value for
 * logical comparisons. For example:
 *     #if MIR_CLIENT_VERSION >= MIR_VERSION_NUMBER(2,3,4)
 *
 * This can be useful to conditionally build code depending on new features or
 * specific bugfixes in the Mir client library.
 */
#define MIR_VERSION_NUMBER(major,minor,micro) \
    (((major) << 22) + ((minor) << 12) + (micro))

#endif /* MIR_VERSION_NUMBER_H_ */
