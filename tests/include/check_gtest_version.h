/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef GMOCK_CHECK_VERSION_H_
#define GMOCK_CHECK_VERSION_H_

// NOTE: GTEST_VERSION_* macros are pulled out of the gtest-config utility in cmake, they are NOT provided by any GTest headers.

#if !defined(GTEST_VERSION_MAJOR) || !defined(GTEST_VERSION_MINOR) || !defined(GTEST_VERSION_PATCH)
#error GTest version macro(s) (GTEST_VERSION_MAJOR, GTEST_VERSION_MINOR or GTEST_VERSION_PATCH) not defined, They should have been defined in tests/CMakeLists.txt
#endif

#ifdef GTEST_VERSION_MAJOR
#if GTEST_VERSION_MAJOR != 1
#error only GTest version 1.x.x supported
#endif
#endif

#define GTEST_AT_LEAST(x, y, z) (GTEST_VERSION_MAJOR > x || (GTEST_VERSION_MAJOR == x && (GTEST_VERSION_MINOR > y || (GTEST_VERSION_MINOR == y && (GTEST_VERSION_PATCH >= z)))))

#endif // GMOCK_CHECK_VERSION_H_
