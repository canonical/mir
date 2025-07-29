/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_ACCESSIBILITY_TEST_SERVER_H
#define MIRAL_ACCESSIBILITY_TEST_SERVER_H

#include "miral/test_server.h"
#include "mir/server.h"

#include "mir/test/doubles/mock_accessibility_manager.h"

namespace mtd = mir::test::doubles;

namespace miral
{
class TestAccessibilityManager : public miral::TestServer
{
public:
    TestAccessibilityManager()
    {
        start_server_in_setup = false;
        add_server_init(
            [this](mir::Server& server)
            {
                server.override_the_accessibility_manager(
                    [&, this]
                    {
                        return accessibility_manager;
                    });
            });
    }

    std::shared_ptr<testing::NiceMock<mtd::MockAccessibilityManager>> accessibility_manager =
        std::make_shared<testing::NiceMock<mtd::MockAccessibilityManager>>();
};

}

#endif // MIRAL_ACCESSIBILITY_TEST_SERVER_H
