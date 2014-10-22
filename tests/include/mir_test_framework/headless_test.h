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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_FRAMEWORK_HEADLESS_TEST_H_
#define MIR_TEST_FRAMEWORK_HEADLESS_TEST_H_

#include "mir_test/temporary_environment_value.h"

#include <gtest/gtest.h>

#include <list>

namespace mir_test_framework
{
/** Basic fixture for tests that don't use graphics hardware.
 *  This provides a mechanism for temporarily setting environment variables.
 *  It automatically sets "MIR_SERVER_PLATFORM_GRAPHICS_LIB" to "libmirplatformstub.so"
 *  as the tests do not hit the graphics hardware.
 */
class HeadlessTest : public ::testing::Test
{
public:
    HeadlessTest();

    void add_to_environment(char const* key, char const* value);

private:
    std::list<mir::test::TemporaryEnvironmentValue> env;
};
}

#endif /* MIR_TEST_FRAMEWORK_HEADLESS_TEST_H_ */
