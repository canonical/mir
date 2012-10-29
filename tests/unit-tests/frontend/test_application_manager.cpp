/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/compositor/buffer_bundle.h"
#include "mir/frontend/application_manager.h"
#include "mir/frontend/application_session_container.h"
#include "mir/surfaces/application_surface_organiser.h"
#include "mir/surfaces/surface.h"
#include "mir_test/mock_buffer_bundle.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::surfaces;

namespace
{

struct MockApplicationSurfaceOrganiser : public ms::ApplicationSurfaceOrganiser
{
    MOCK_METHOD1(create_surface, std::weak_ptr<ms::Surface>(const ms::SurfaceCreationParameters&));
    MOCK_METHOD1(destroy_surface, void(std::weak_ptr<ms::Surface> surface));
};

  struct MockApplicationSessionModel : public mf::ApplicationSessionContainer
  {
    MOCK_METHOD1(insert_session, void(std::shared_ptr<mf::ApplicationSession>));
    MOCK_METHOD1(remove_session, void(std::shared_ptr<mf::ApplicationSession>));
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
    MOCK_METHOD0(iterator, std::shared_ptr<mf::ApplicationSessionContainer::LockingIterator>());
  };

  struct MockFocusStrategy: public mf::ApplicationFocusStrategy
  {
    MOCK_METHOD1(next_focus_app, std::weak_ptr<mf::ApplicationSession>(std::shared_ptr<mf::ApplicationSession>));
  };

}

TEST(ApplicationManager, open_and_close_session)
{
    using namespace ::testing;
    MockApplicationSurfaceOrganiser organizer;
    MockApplicationSessionModel *model = new MockApplicationSessionModel();
    MockFocusStrategy *strategy = new MockFocusStrategy();

    mf::ApplicationManager app_manager(&organizer, model, strategy);

    EXPECT_CALL(*model, insert_session(_)).Times(1);
    EXPECT_CALL(*model, remove_session(_)).Times(1);
    auto session = app_manager.open_session();
    app_manager.close_session(session.lock());
}
