/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_FRAMEWORK_HEADLESS_IN_PROCESS_SERVER_H_
#define MIR_TEST_FRAMEWORK_HEADLESS_IN_PROCESS_SERVER_H_

#include <mir_test_framework/headless_test.h>

namespace mir_test_framework
{
struct HeadlessInProcessServer : HeadlessTest
{
    HeadlessInProcessServer();

    void SetUp() override;

    void TearDown() override;
};
}

#endif /* MIR_TEST_FRAMEWORK_HEADLESS_IN_PROCESS_SERVER_H_ */
