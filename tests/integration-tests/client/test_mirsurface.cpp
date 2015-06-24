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
#include "mir/test/doubles/stub_scene_surface.h"
#include "mir/test/fake_shared.h"
#include "mir/test/validity_matchers.h"

#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/shell/shell_wrapper.h"
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
struct MockShell : msh::ShellWrapper
{
    using msh::ShellWrapper::ShellWrapper;
    MOCK_METHOD2(create_surface, mf::SurfaceId(std::shared_ptr<ms::Session> const&, ms::SurfaceCreationParameters const&));
};

bool parent_field_matches(ms::SurfaceCreationParameters const& params,
    mir::optional_value<MirSurface*> const& surf)
{
    if (!surf.is_set())
        return !params.parent_id.is_set();

    return surf.value()->id() == params.parent_id.value().as_value();
}

MATCHER_P(MatchesRequired, spec, "")
{
    return spec->width == arg.size.width.as_int() &&
           spec->height == arg.size.height.as_int() &&
           spec->pixel_format == arg.pixel_format &&
           spec->buffer_usage == static_cast<MirBufferUsage>(arg.buffer_usage);
}

MATCHER_P(MatchesOptional, spec, "")
{
    return spec->surface_name == arg.name &&
           spec->state == arg.state &&
           spec->type == arg.type &&
           spec->pref_orientation == arg.preferred_orientation &&
           spec->output_id == static_cast<uint32_t>(arg.output_id.as_value()) &&
           parent_field_matches(arg, spec->parent);
}

MATCHER(IsAMenu, "")
{
    return arg.type == mir_surface_type_menu;
}

MATCHER(IsATooltip, "")
{
    return arg.type == mir_surface_type_tip;
}

MATCHER(IsADialog, "")
{
    return arg.type == mir_surface_type_dialog;
}

MATCHER_P(HasParent, parent, "")
{
    return arg.parent_id.is_set() && arg.parent_id.value().as_value() == parent->id();
}

MATCHER(NoParentSet, "")
{
    return arg.parent_id.is_set() == false;
}

MATCHER_P(MatchesAuxRect, rect, "")
{
    return arg.aux_rect.is_set() &&
           arg.aux_rect.value().top_left.x.as_int() == rect.left &&
           arg.aux_rect.value().top_left.y.as_int() == rect.top &&
           arg.aux_rect.value().size.width.as_uint32_t() == rect.width &&
           arg.aux_rect.value().size.height.as_uint32_t() == rect.height;
}

MATCHER_P(MatchesEdge, edge, "")
{
    return arg.edge_attachment.is_set() &&
           arg.edge_attachment.value() == edge;
}

}

struct ClientMirSurface : mtf::ConnectedClientHeadlessServer
{
    void SetUp() override
    {
        server.wrap_shell([this]
            (std::shared_ptr<msh::Shell> const& wrapped)
            {
                wrapped_shell = wrapped;
                auto const msc = std::make_shared<MockShell>(wrapped);
                mock_shell = msc;
                return msc;
            });
        mtf::ConnectedClientHeadlessServer::SetUp();

        server.the_surface_coordinator();

        ON_CALL(*mock_shell, create_surface(_, _))
            .WillByDefault(Invoke(wrapped_shell.get(), &msh::Shell::create_surface));

        spec.connection = connection;
        spec.buffer_usage = mir_buffer_usage_software;
        spec.surface_name = "test_surface";
        spec.output_id = mir_display_output_id_invalid;
        spec.type = mir_surface_type_dialog;
        spec.state = mir_surface_state_minimized;
        spec.pref_orientation = mir_orientation_mode_landscape;
    }

    std::unique_ptr<MirSurface, std::function<void(MirSurface*)>> create_surface(MirSurfaceSpec* spec)
    {
        return {mir_surface_create_sync(spec), [](MirSurface *surf){mir_surface_release_sync(surf);}};
    }

    MirSurfaceSpec spec{nullptr, 640, 480, mir_pixel_format_abgr_8888};
    std::shared_ptr<msh::Shell> wrapped_shell;
    std::shared_ptr<MockShell> mock_shell;
};

TEST_F(ClientMirSurface, sends_optional_params)
{
    EXPECT_CALL(*mock_shell, create_surface(_,AllOf(MatchesRequired(&spec), MatchesOptional(&spec))));

    auto surf = create_surface(&spec);

    // A valid surface is needed to be specified as a parent
    ASSERT_THAT(surf.get(), IsValid());
    spec.parent = surf.get();

    EXPECT_CALL(*mock_shell, create_surface(_, MatchesOptional(&spec)));
    create_surface(&spec);
}

TEST_F(ClientMirSurface, as_menu_sends_correct_params)
{
    EXPECT_CALL(*mock_shell, create_surface(_,_));
    auto parent = create_surface(&spec);
    ASSERT_THAT(parent.get(), IsValid());

    MirRectangle attachment_rect{100, 200, 300, 400};
    MirEdgeAttachment edge{mir_edge_attachment_horizontal};

    auto spec_deleter = [](MirSurfaceSpec* spec) {mir_surface_spec_release(spec);};
    std::unique_ptr<MirSurfaceSpec, decltype(spec_deleter)> menu_spec{
        mir_connection_create_spec_for_menu(connection, 640, 480,
            mir_pixel_format_abgr_8888, parent.get(), &attachment_rect,
            edge),
        spec_deleter
    };

    ASSERT_THAT(menu_spec, NotNull());

    EXPECT_CALL(*mock_shell, create_surface(_, AllOf(IsAMenu(),
        HasParent(parent.get()), MatchesAuxRect(attachment_rect), MatchesEdge(edge))));
    create_surface(menu_spec.get());
}

TEST_F(ClientMirSurface, as_tooltip_sends_correct_params)
{
    EXPECT_CALL(*mock_shell, create_surface(_,_));
    auto parent = create_surface(&spec);
    ASSERT_THAT(parent.get(), IsValid());

    MirRectangle target_zone_rect{100, 200, 300, 400};

    auto spec_deleter = [](MirSurfaceSpec* spec) {mir_surface_spec_release(spec);};
    std::unique_ptr<MirSurfaceSpec, decltype(spec_deleter)> tooltip_spec{
        mir_connection_create_spec_for_tooltip(connection, 640, 480,
            mir_pixel_format_abgr_8888, parent.get(), &target_zone_rect),
        spec_deleter
    };

    ASSERT_THAT(tooltip_spec, NotNull());

    EXPECT_CALL(*mock_shell, create_surface(_, AllOf(IsATooltip(),
        HasParent(parent.get()), MatchesAuxRect(target_zone_rect))));
    create_surface(tooltip_spec.get());
}

TEST_F(ClientMirSurface, as_dialog_sends_correct_params)
{
    auto spec_deleter = [](MirSurfaceSpec* spec) {mir_surface_spec_release(spec);};
    std::unique_ptr<MirSurfaceSpec, decltype(spec_deleter)> dialog_spec{
        mir_connection_create_spec_for_dialog(connection, 640, 480, mir_pixel_format_abgr_8888),
        spec_deleter
    };

    ASSERT_THAT(dialog_spec, NotNull());

    EXPECT_CALL(*mock_shell, create_surface(_, AllOf(IsADialog(), NoParentSet())));
    create_surface(dialog_spec.get());
}

TEST_F(ClientMirSurface, as_modal_dialog_sends_correct_params)
{
    EXPECT_CALL(*mock_shell, create_surface(_,_));
    auto parent = create_surface(&spec);
    ASSERT_THAT(parent.get(), IsValid());

    auto spec_deleter = [](MirSurfaceSpec* spec) {mir_surface_spec_release(spec);};
    std::unique_ptr<MirSurfaceSpec, decltype(spec_deleter)> dialog_spec{
        mir_connection_create_spec_for_modal_dialog(connection, 640, 480,
            mir_pixel_format_abgr_8888, parent.get()),
        spec_deleter
    };

    ASSERT_THAT(dialog_spec, NotNull());

    EXPECT_CALL(*mock_shell, create_surface(_, AllOf(IsADialog(), HasParent(parent.get()))));
    create_surface(dialog_spec.get());
}
