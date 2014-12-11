/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "mir_test_framework/connected_client_headless_server.h"
#include "mir_test_doubles/stub_scene_surface.h"
#include "mir_test/fake_shared.h"

#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/shell/surface_coordinator_wrapper.h"
#include "src/client/mir_surface.h"
#include "src/include/common/mir/raii.h"

#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

using namespace testing;

namespace
{
struct MockSurfaceCoordinator : msh::SurfaceCoordinatorWrapper
{
    using msh::SurfaceCoordinatorWrapper::SurfaceCoordinatorWrapper;
    MOCK_METHOD2(add_surface, std::shared_ptr<ms::Surface>(ms::SurfaceCreationParameters const&, ms::Session*));
};

bool parent_field_matches(ms::SurfaceCreationParameters const& params, MirSurface* surf)
{
    if (surf == nullptr)
        return params.has_parent == false;

    return surf->id() == params.parent_id.as_value();
}

MATCHER_P(MatchesSpec, spec, "")
{
    return spec->name == arg.name &&
           spec->width == arg.size.width.as_int() &&
           spec->height == arg.size.height.as_int() &&
           spec->state == arg.state &&
           spec->type == arg.type &&
           spec->preferred_orientation == arg.preferred_orientation &&
           static_cast<int>(spec->output_id) == arg.output_id.as_value() &&
           spec->pixel_format == arg.pixel_format &&
           static_cast<mg::BufferUsage>(spec->buffer_usage) == arg.buffer_usage &&
           parent_field_matches(arg, spec->parent);
}

}

struct ClientMirSurface : mtf::ConnectedClientHeadlessServer
{
    void SetUp() override
    {
        server.wrap_surface_coordinator([this]
            (std::shared_ptr<ms::SurfaceCoordinator> const& wrapped)
            {
                wrapped_coord = wrapped;
                auto const msc = std::make_shared<MockSurfaceCoordinator>(wrapped);
                mock_surface_coordinator = msc;
                return msc;
            });
        mtf::ConnectedClientHeadlessServer::SetUp();

        server.the_surface_coordinator();

        ON_CALL(*mock_surface_coordinator, add_surface(_, _))
            .WillByDefault(Invoke(wrapped_coord.get(), &ms::SurfaceCoordinator::add_surface));

        spec.connection = connection;
        spec.width = 640;
        spec.height = 480;
        spec.buffer_usage = mir_buffer_usage_software;
        spec.state = mir_surface_state_minimized;
        spec.type = mir_surface_type_dialog;
        spec.preferred_orientation = mir_orientation_mode_landscape;
        spec.parent = nullptr;
        spec.name = "test_surface";
    }

    std::unique_ptr<MirSurface, std::function<void(MirSurface*)>> create_surface()
    {
        return {mir_surface_create_sync(&spec), [](MirSurface *surf){mir_surface_release_sync(surf);}};
    }

    MirSurfaceSpec spec;
    std::shared_ptr<ms::SurfaceCoordinator> wrapped_coord;
    std::shared_ptr<MockSurfaceCoordinator> mock_surface_coordinator;
};

TEST_F(ClientMirSurface, sends_all_mirspec_params_to_server)
{
    EXPECT_CALL(*mock_surface_coordinator, add_surface(MatchesSpec(&spec),_)).Times(1);

    auto surf = create_surface();

    // A valid surface is needed to be specified as a parent
    ASSERT_TRUE(mir_surface_is_valid(surf.get()));
    spec.parent = surf.get();

    // The second time around we don't care if the surface gets created,
    // but we'd like to validate that the server received the output id specified
    int const arbitrary_output_id = 3000;
    spec.output_id = arbitrary_output_id;

    EXPECT_CALL(*mock_surface_coordinator, add_surface(MatchesSpec(&spec),_)).Times(1);
    create_surface();
}
