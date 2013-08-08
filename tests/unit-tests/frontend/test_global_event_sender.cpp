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

#include "src/server/frontend/message_sender.h"
#include "src/server/frontend/event_sender.h"
#include "mir_test_doubles/stub_display_configuration.h"
#include "mir_test/display_config_matchers.h"
#include "mir_test/fake_shared.h"

#include <vector>
#include "mir_protobuf.pb.h"
#include "mir_protobuf_wire.pb.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace mfd=mir::frontend::detail;
namespace geom=mir::geometry;

class MockSessionStorage : public msh::SessionContainer
{
    MOCK_METHOD1(insert_session, void(std::shared_ptr<Session> const&));
    MOCK_METHOD1(remove_session, void(std::shared_ptr<Session> const&));
    MOCK_CONST_METHOD1(for_each, void(std::function<void(std::shared_ptr<msh::Session> const&)>));
}


TEST_F(GlobalEventSender, sender)
{
    MockSenssionStorage mock_storage;

    std::function<void(std::shared_ptr<msh::Session> const&)> called_fn;

    EXPECT_CALL(mock_storage, for_each(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&called_fn)); 

    GlobalEventSender g_sender(mt::fake_shared(mock_storage));

    g_sender.send_display_configuration(config);

    mtd::MockShellSurface surface;
    EXPECT_CALL(surface, send_display_configuration(_))
        .Times(1);
    called_fn(mt::fake_shared(surface));
}
