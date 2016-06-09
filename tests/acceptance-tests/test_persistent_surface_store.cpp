/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored By: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "mir/scene/surface.h"
#include "mir/shell/persistent_surface_store.h"

#include "mir/test/doubles/wrap_shell_to_track_latest_surface.h"
#include "mir_test_framework/connected_client_headless_server.h"
#include "mir/test/fake_shared.h"
#include "mir_test_framework/any_surface.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mtf = mir_test_framework;

using namespace testing;

namespace
{
struct TestPersistentSurfaceStore : mtf::ConnectedClientHeadlessServer
{
    void SetUp() override
    {
        server.wrap_shell([this](std::shared_ptr<msh::Shell> const& wrapped)
        {
            auto const msc = std::make_shared<mtd::WrapShellToTrackLatestSurface>(wrapped);
            shell = msc;
            return msc;
        });

        mtf::ConnectedClientHeadlessServer::SetUp();
    }

    void TearDown() override
    {
        shell.reset();
        mtf::ConnectedClientHeadlessServer::TearDown();
    }

    std::shared_ptr<ms::Surface> latest_shell_surface() const
    {
        auto const result = shell->latest_surface.lock();
        EXPECT_THAT(result, NotNull());
        return result;
    }

private:
    std::shared_ptr<mtd::WrapShellToTrackLatestSurface> shell;
};
}

TEST_F(TestPersistentSurfaceStore, server_and_client_persistent_id_matches)
{
    auto const client_surface = mtf::make_any_surface(connection);
    ASSERT_THAT(client_surface, NotNull());

    auto const shell_surface = latest_shell_surface();
    ASSERT_THAT(shell_surface, NotNull());

    MirPersistentId* client_surface_id = mir_surface_request_persistent_id_sync(client_surface);
    msh::PersistentSurfaceStore::Id server_surface_id = server.the_persistent_surface_store()->id_for_surface(shell_surface);

    std::string client_surface_id_string(mir_persistent_id_as_string(client_surface_id));

    ASSERT_EQ(server_surface_id.serialize_to_string(), client_surface_id_string);

    mir_persistent_id_release(client_surface_id);
    mir_surface_release_sync(client_surface);
}
