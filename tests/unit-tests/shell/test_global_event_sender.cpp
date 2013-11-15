/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "src/server/surfaces/global_event_sender.h"
#include "src/server/surfaces/session_container.h"
#include "mir_test_doubles/mock_frontend_surface.h"
#include "mir_test_doubles/mock_shell_session.h"
#include "mir_test_doubles/stub_display_configuration.h"
#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace ms = mir::surfaces;
namespace msh=mir::shell;

namespace
{
class MockSessionStorage : public ms::SessionContainer
{
public:
    MOCK_METHOD1(insert_session, void(std::shared_ptr<msh::Session> const&));
    MOCK_METHOD1(remove_session, void(std::shared_ptr<msh::Session> const&));
    MOCK_CONST_METHOD1(for_each, void(std::function<void(std::shared_ptr<msh::Session> const&)>));
    MOCK_CONST_METHOD1(successor_of, std::shared_ptr<msh::Session>(std::shared_ptr<msh::Session> const&));
};
}

TEST(GlobalEventSender, sender)
{
    using namespace testing;

    MockSessionStorage mock_storage;

    std::function<void(std::shared_ptr<msh::Session> const&)> called_fn;

    EXPECT_CALL(mock_storage, for_each(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&called_fn)); 

    ms::GlobalEventSender g_sender(mt::fake_shared(mock_storage));

    mtd::StubDisplayConfig stub_display_config;
    g_sender.handle_display_config_change(stub_display_config);

    auto mock_session = std::make_shared<mtd::MockShellSession>();
    EXPECT_CALL(*mock_session, send_display_config(_))
        .Times(1);
    called_fn(mock_session);
}
