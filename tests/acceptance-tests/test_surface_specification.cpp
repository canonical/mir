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

#include "mir/scene/null_surface_observer.h"
#include "mir/scene/surface.h"

#include "mir/test/doubles/wrap_shell_to_track_latest_surface.h"
#include "mir_test_framework/connected_client_headless_server.h"
#include "mir/test/fake_shared.h"
#include "mir/test/signal.h"
#include "mir_toolkit/common.h"
#include "mir_toolkit/client_types.h"
#include "../../include/common/mir_toolkit/common.h"

namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace mir::geometry;
using namespace testing;
using namespace std::chrono_literals;

namespace
{
class SurfaceHandle
{
public:
    explicit SurfaceHandle(MirSurface* surface) : surface{surface} {}
    ~SurfaceHandle() { if (surface) mir_surface_release_sync(surface); }

    operator MirSurface*() const { return surface; }
    SurfaceHandle(SurfaceHandle&& that) : surface{that.surface} { that.surface = nullptr; }
private:
    SurfaceHandle(SurfaceHandle const&) = delete;
    MirSurface* surface;
};

class MockSurfaceObserver : public ms::NullSurfaceObserver
{
public:
    MOCK_METHOD1(renamed, void(char const*));
    MOCK_METHOD2(attrib_changed, void(MirSurfaceAttrib attrib, int value));
};

struct SurfaceSpecification : mtf::ConnectedClientHeadlessServer
{
    SurfaceSpecification() { add_to_environment("MIR_SERVER_ENABLE_INPUT", "OFF"); }

    Rectangle const first_display {{  0, 0}, {640,  480}};
    Rectangle const second_display{{640, 0}, {640,  480}};

    void SetUp() override
    {
        initial_display_layout({first_display, second_display});

        server.wrap_shell([this](std::shared_ptr<msh::Shell> const& wrapped)
              {
                  auto const msc = std::make_shared<mtd::WrapShellToTrackLatestSurface>(wrapped);
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
    SurfaceHandle create_surface(Specifier const& specifier) const
    {
        auto const spec = mir_create_surface_spec(connection);

        specifier(spec);

        auto const surface = mir_surface_create_sync(spec);
        mir_surface_spec_release(spec);

        return SurfaceHandle{surface};
    }

    NiceMock<MockSurfaceObserver> surface_observer;

    void change_observed() { signal_change.raise(); }
    MirPixelFormat pixel_format{mir_pixel_format_invalid};
    static auto const width = 97;
    static auto const height= 101;

private:
    std::shared_ptr<mtd::WrapShellToTrackLatestSurface> shell;
    mt::Signal signal_change;

    void init_pixel_format()
    {
        unsigned int valid_formats{0};
        MirPixelFormat pixel_formats[mir_pixel_formats];
        mir_connection_get_available_surface_formats(connection, pixel_formats, mir_pixel_formats, &valid_formats);

        pixel_format = pixel_formats[0];
    }
};

struct SurfaceSpecificationCase : SurfaceSpecification, ::testing::WithParamInterface<MirSurfaceType> {};
using SurfaceWithoutParent = SurfaceSpecificationCase;
using SurfaceNeedingParent = SurfaceSpecificationCase;
using SurfaceMayHaveParent = SurfaceSpecificationCase;
}

MATCHER(IsValidSurface, "")
{
    if (arg == nullptr) return false;

    if (!mir_surface_is_valid(arg)) return false;

    return true;
}

TEST_P(SurfaceWithoutParent, not_setting_parent_succeeds)
{
    auto const type = GetParam();

    auto const surface = create_surface([&](MirSurfaceSpec* spec)
        {
            mir_surface_spec_set_type(spec, type);
            mir_surface_spec_set_width(spec, width);
            mir_surface_spec_set_height(spec, height);
            mir_surface_spec_set_pixel_format(spec, pixel_format);
            mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
        });

    EXPECT_THAT(surface, IsValidSurface());
}

TEST_P(SurfaceWithoutParent, setting_parent_fails)
{
    auto const type = GetParam();

    auto const parent = create_surface([&](MirSurfaceSpec* spec)
        {
            mir_surface_spec_set_type(spec, mir_surface_type_normal);
            mir_surface_spec_set_width(spec, width);
            mir_surface_spec_set_height(spec, height);
            mir_surface_spec_set_pixel_format(spec, pixel_format);
            mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
        });

    auto const surface = create_surface([&](MirSurfaceSpec* spec)
    {
        mir_surface_spec_set_type(spec, type);
        mir_surface_spec_set_width(spec, width);
        mir_surface_spec_set_height(spec, height);
        mir_surface_spec_set_pixel_format(spec, pixel_format);
        mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
        mir_surface_spec_set_parent(spec, parent);
    });

    EXPECT_THAT(surface, Not(IsValidSurface()));
}

TEST_P(SurfaceNeedingParent, setting_parent_succeeds)
{
    auto const type = GetParam();

    auto const parent = create_surface([&](MirSurfaceSpec* spec)
        {
            mir_surface_spec_set_type(spec, mir_surface_type_normal);
            mir_surface_spec_set_width(spec, width);
            mir_surface_spec_set_height(spec, height);
            mir_surface_spec_set_pixel_format(spec, pixel_format);
            mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
        });

    auto const surface = create_surface([&](MirSurfaceSpec* spec)
        {
            mir_surface_spec_set_type(spec, type);
            mir_surface_spec_set_width(spec, width);
            mir_surface_spec_set_height(spec, height);
            mir_surface_spec_set_pixel_format(spec, pixel_format);
            mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
            mir_surface_spec_set_parent(spec, parent);
        });

    EXPECT_THAT(surface, IsValidSurface());
}

TEST_P(SurfaceNeedingParent, not_setting_parent_fails)
{
    auto const type = GetParam();

    auto const surface = create_surface([&](MirSurfaceSpec* spec)
        {
            mir_surface_spec_set_type(spec, type);
            mir_surface_spec_set_width(spec, width);
            mir_surface_spec_set_height(spec, height);
            mir_surface_spec_set_pixel_format(spec, pixel_format);
            mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
        });

    EXPECT_THAT(surface, Not(IsValidSurface()));
}

TEST_P(SurfaceMayHaveParent, setting_parent_succeeds)
{
    auto const type = GetParam();

    auto const parent = create_surface([&](MirSurfaceSpec* spec)
        {
            mir_surface_spec_set_type(spec, mir_surface_type_normal);
            mir_surface_spec_set_width(spec, width);
            mir_surface_spec_set_height(spec, height);
            mir_surface_spec_set_pixel_format(spec, pixel_format);
            mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
        });

    auto const surface = create_surface([&](MirSurfaceSpec* spec)
        {
            mir_surface_spec_set_type(spec, type);
            mir_surface_spec_set_width(spec, width);
            mir_surface_spec_set_height(spec, height);
            mir_surface_spec_set_pixel_format(spec, pixel_format);
            mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
            mir_surface_spec_set_parent(spec, parent);
        });

    EXPECT_THAT(surface, IsValidSurface());
}

TEST_P(SurfaceMayHaveParent, not_setting_parent_succeeds)
{
    auto const type = GetParam();

    auto const surface = create_surface([&](MirSurfaceSpec* spec)
        {
            mir_surface_spec_set_type(spec, type);
            mir_surface_spec_set_width(spec, width);
            mir_surface_spec_set_height(spec, height);
            mir_surface_spec_set_pixel_format(spec, pixel_format);
            mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
        });

    EXPECT_THAT(surface, IsValidSurface());
}

INSTANTIATE_TEST_CASE_P(SurfaceSpecification, SurfaceWithoutParent,
    Values(mir_surface_type_utility, mir_surface_type_normal));

INSTANTIATE_TEST_CASE_P(SurfaceSpecification, SurfaceNeedingParent,
    Values(mir_surface_type_satellite, mir_surface_type_gloss, mir_surface_type_tip));

INSTANTIATE_TEST_CASE_P(SurfaceSpecification, SurfaceMayHaveParent,
    Values(mir_surface_type_dialog, mir_surface_type_freestyle));
