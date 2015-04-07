/*
 * Copyright © 2015 Canonical Ltd.
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

#include "mir_test_framework/connected_client_with_a_surface.h"

#include "mir/shell/shell_wrapper.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/null_surface_observer.h"

#include "mir_test/fake_shared.h"

namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mt = mir::test;

using namespace testing;

namespace
{
class MockSurfaceObserver : public ms::NullSurfaceObserver
{
public:
    MOCK_METHOD1(renamed, void(char const*));
};

struct StubShell : msh::ShellWrapper
{
    using msh::ShellWrapper::ShellWrapper;

    mf::SurfaceId create_surface(
        std::shared_ptr<ms::Session> const& session,
        ms::SurfaceCreationParameters const& params) override
    {
        auto const surface = msh::ShellWrapper::create_surface(session, params);
        latest_surface = session->surface(surface);
        return surface;
    }

    std::weak_ptr<ms::Surface> latest_surface;
};

struct SurfaceModifications : mtf::ConnectedClientWithASurface
{
    SurfaceModifications() { add_to_environment("MIR_SERVER_ENABLE_INPUT", "OFF"); }

    void SetUp() override
    {
        std::shared_ptr<StubShell> shell;

        server.wrap_shell([&](std::shared_ptr<msh::Shell> const& wrapped)
        {
            auto const msc = std::make_shared<StubShell>(wrapped);
            shell = msc;
            return msc;
        });

        mtf::ConnectedClientWithASurface::SetUp();

        auto const scene_surface = shell->latest_surface.lock();
        scene_surface->add_observer(mt::fake_shared(surface_observer));
    }

    MockSurfaceObserver surface_observer;
};
}

TEST_F(SurfaceModifications, rename_is_notified)
{
    auto const new_title = __PRETTY_FUNCTION__;

    EXPECT_CALL(surface_observer, renamed(StrEq(new_title)));

    mir_surface_set_title(surface, new_title);
}
