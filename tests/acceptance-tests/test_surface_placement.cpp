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

#include "mir_test_framework/connected_client_headless_server.h"

#include "mir/shell/shell_wrapper.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"

#include "mir_test/fake_shared.h"
#include "mir_test/signal.h"

namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mt = mir::test;

using namespace mir::geometry;
using namespace testing;

namespace
{
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

struct SurfacePlacement : mtf::ConnectedClientHeadlessServer
{
    SurfacePlacement() { add_to_environment("MIR_SERVER_ENABLE_INPUT", "OFF"); }

    Rectangle const first_display {{0, 0}, {640,  480}};
    Rectangle const second_display{{0, 0}, {640,  480}};

    void SetUp() override
    {
        initial_display_layout({first_display, second_display});

        server.wrap_shell([this](std::shared_ptr<msh::Shell> const& wrapped)
        {
            auto const msc = std::make_shared<StubShell>(wrapped);
            shell = msc;
            return msc;
        });

        mtf::ConnectedClientHeadlessServer::SetUp();

        init_pixel_format();
    }

    void TearDown() override
    {
        shell.reset();
        mtf::ConnectedClientHeadlessServer::TearDown();
    }

    std::shared_ptr<ms::Surface> latest_shell_surface() const
    {
        auto const result = shell->latest_surface.lock();
//      ASSERT_THAT(result, NotNull()); //<= doesn't compile!?
        EXPECT_THAT(result, NotNull());
        return result;
    }

    template<typename Specifier>
    MirSurface* create_normal_surface(int width, int height, Specifier const& specifier) const
    {
        auto const spec = mir_connection_create_spec_for_normal_surface(
            connection, width, height, pixel_format);

        specifier(spec);

        auto const surface = mir_surface_create_sync(spec);
        mir_surface_spec_release(spec);

        return surface;
    }

    MirSurface* create_normal_surface(int width, int height) const
    {
        return create_normal_surface(width, height, [](MirSurfaceSpec*){});
    }

private:
    std::shared_ptr<StubShell> shell;
    MirPixelFormat pixel_format{mir_pixel_format_invalid};

    void init_pixel_format()
    {
        unsigned int valid_formats
        { 0 };
        MirPixelFormat pixel_formats[mir_pixel_formats];
        mir_connection_get_available_surface_formats(connection, pixel_formats, mir_pixel_formats, &valid_formats);
        //select an 8 bit opaque format if we can
        for (auto i = 0u; i < valid_formats; i++)
        {
            if (pixel_formats[i] == mir_pixel_format_xbgr_8888 || pixel_formats[i] == mir_pixel_format_xrgb_8888)
            {
                pixel_format = pixel_formats[i];
                break;
            }
        }
    }
};
}

// Optically centered in a space means:
//
//  o horizontally centered, and positioned vertically such that the top margin
//    is half the bottom margin (vertical centering would look too low, and
//    would allow little room for cascading), unless this would leave any
//    part of the window off-screen or in shell space;
//
//  o otherwise, as close as possible to that position without any part of the
//    window being off-screen or in shell space, if possible;
//
//  o otherwise, as close as possible to that position while keeping all of its
//    title bar thickness in non-shell space. (For example, a dialog that is
//    taller than the screen should be positioned immediately below any menu
//    bar or shell panel at the top of the screen.)

TEST_F(SurfacePlacement, small_window_is_optically_centered_on_first_display)
{
    auto const width = 59;
    auto const height= 61;

    auto const geometric_centre = first_display.top_left +
        0.5*(as_displacement(first_display.size) - Displacement{width, height});

    auto const optically_centred = geometric_centre -
        DeltaY{(first_display.size.height.as_int()-height)/6};

    auto const surface = create_normal_surface(width, height);
    auto const shell_surface = latest_shell_surface();
    ASSERT_THAT(shell_surface, NotNull());  // Compiles here

    EXPECT_THAT(shell_surface->top_left(), Eq(optically_centred));
    EXPECT_THAT(shell_surface->size(),     Eq(Size{width, height}));

    mir_surface_release_sync(surface);
}

TEST_F(SurfacePlacement, medium_window_fitted_onto_first_display)
{
    auto const width = first_display.size.width.as_int();
    auto const height= first_display.size.height.as_int();

    auto const surface = create_normal_surface(width, height);
    auto const shell_surface = latest_shell_surface();

    EXPECT_THAT(shell_surface->top_left(), Eq(first_display.top_left));
    EXPECT_THAT(shell_surface->size(),     Eq(Size{width, height}));
    EXPECT_THAT(shell_surface->size(),     Eq(first_display.size));

    mir_surface_release_sync(surface);
}

TEST_F(SurfacePlacement, big_window_keeps_top_on_first_display)
{
    auto const width = 2*first_display.size.width.as_int();
    auto const height= 2*first_display.size.height.as_int();

    auto const surface = create_normal_surface(width, height);
    auto const shell_surface = latest_shell_surface();

    EXPECT_THAT(shell_surface->top_left(), Eq(Point{-width/4, 0}));
    EXPECT_THAT(shell_surface->size(),     Eq(Size{width, height}));

    mir_surface_release_sync(surface);
}
