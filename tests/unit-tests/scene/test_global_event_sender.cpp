/*
 * Copyright © 2013-2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "src/server/scene/global_event_sender.h"
#include "mir/scene/session_container.h"
#include "mir/test/doubles/mock_scene_session.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace ms = mir::scene;

TEST(GlobalEventSender, display_config_change_is_propagated)
{
    using namespace testing;

    ms::SessionContainer storage;

    std::function<void(std::shared_ptr<ms::Session> const&)> called_fn;

    auto mock_session = std::make_shared<mtd::MockSceneSession>();
    EXPECT_CALL(*mock_session, send_display_config(_))
        .Times(1);

    storage.insert_session(mock_session);
    ms::GlobalEventSender g_sender(mt::fake_shared(storage));

    mtd::StubDisplayConfig stub_display_config;
    g_sender.handle_display_config_change(stub_display_config);
}
