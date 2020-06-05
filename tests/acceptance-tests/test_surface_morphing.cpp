/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include <gmock/gmock.h>

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
    explicit SurfaceHandle(MirWindow* window) : window{window} {}
    ~SurfaceHandle() { if (window) mir_window_release_sync(window); }

    operator MirWindow*() const { return window; }
    SurfaceHandle(SurfaceHandle&& that) : window{that.window} { that.window = nullptr; }
private:
    SurfaceHandle(SurfaceHandle const&) = delete;
    MirWindow* window;
};

class MockSurfaceObserver : public ms::NullSurfaceObserver
{
public:
    MOCK_METHOD2(renamed, void(ms::Surface const*, char const*));
    MOCK_METHOD3(attrib_changed, void(ms::Surface const*, MirWindowAttrib attrib, int value));
};

struct SurfaceMorphing : mtf::ConnectedClientHeadlessServer
{
    SurfaceMorphing() { add_to_environment("MIR_SERVER_ENABLE_INPUT", "OFF"); }

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
        auto const spec = mir_create_window_spec(connection);

        specifier(spec);

        auto const window = mir_create_window_sync(spec);
        mir_window_spec_release(spec);

        return SurfaceHandle{window};
    }

    template<typename Specifier>
    void change_surface(MirWindow* window, Specifier const& specifier)
    {
        signal_change.reset();

        auto const spec = mir_create_window_spec(connection);

        specifier(spec);

        mir_window_apply_spec(window, spec);
        mir_window_spec_release(spec);
        signal_change.wait_for(1s);
    }

    void wait_for_arbitrary_change(MirWindow* window)
    {
        auto const new_title = __PRETTY_FUNCTION__;

        EXPECT_CALL(surface_observer, renamed(_, StrEq(new_title))).
            WillOnce(InvokeWithoutArgs([&]{ change_observed(); }));

        change_surface(window, [&](MirWindowSpec* spec)
            {
                mir_window_spec_set_name(spec, new_title);
            });
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
        mir_connection_get_available_surface_formats(connection, &pixel_format, 1, &valid_formats);
    }
};

struct TypePair
{
    MirWindowType from;
    MirWindowType to;

    friend std::ostream& operator<<(std::ostream& out, TypePair const& types)
        { return out << "from:" << types.from << ", to:" << types.to; }
};

struct SurfaceMorphingCase : SurfaceMorphing, ::testing::WithParamInterface<TypePair> {};
using TargetWithoutParent = SurfaceMorphingCase;
using TargetNeedingParent = SurfaceMorphingCase;
using TargetMayHaveParent = SurfaceMorphingCase;
}

TEST_P(TargetWithoutParent, not_setting_parent_succeeds)
{
    auto const old_type = GetParam().from;
    auto const new_type = GetParam().to;

    auto const window = create_surface([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_type(spec, old_type);
            mir_window_spec_set_width(spec, width);
            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            mir_window_spec_set_pixel_format(spec, pixel_format);
            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);
#pragma GCC diagnostic pop
        });

    latest_shell_surface()->add_observer(mt::fake_shared(surface_observer));

    EXPECT_CALL(surface_observer, attrib_changed(_, mir_window_attrib_type, new_type)).
        WillOnce(InvokeWithoutArgs([&] { change_observed(); }));

    change_surface(window, [&](MirWindowSpec* spec)
        {
            mir_window_spec_set_type(spec, new_type);
        });
}

TEST_P(TargetWithoutParent, setting_parent_fails)
{
    auto const old_type = GetParam().from;
    auto const new_type = GetParam().to;

    auto const parent = create_surface([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_type(spec, mir_window_type_normal);
            mir_window_spec_set_width(spec, width);
            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            mir_window_spec_set_pixel_format(spec, pixel_format);
            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);
#pragma GCC diagnostic pop
        });

    auto const window = create_surface([&](MirWindowSpec* spec)
    {
        mir_window_spec_set_type(spec, old_type);
        mir_window_spec_set_width(spec, width);
        mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_window_spec_set_pixel_format(spec, pixel_format);
        mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);
#pragma GCC diagnostic pop
    });

    latest_shell_surface()->add_observer(mt::fake_shared(surface_observer));

    EXPECT_CALL(surface_observer, attrib_changed(_, mir_window_attrib_type, new_type)).
        Times(0);

    change_surface(window, [&](MirWindowSpec* spec)
        {
            mir_window_spec_set_type(spec, new_type);
            mir_window_spec_set_parent(spec, parent);

            // Don't wait for a notification we don't expect
            // We'll wait for another change
            change_observed();
        });

    wait_for_arbitrary_change(window);
}

TEST_P(TargetNeedingParent, setting_parent_succeeds)
{
    auto const old_type = GetParam().from;
    auto const new_type = GetParam().to;

    auto const parent = create_surface([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_type(spec, mir_window_type_normal);
            mir_window_spec_set_width(spec, width);
            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            mir_window_spec_set_pixel_format(spec, pixel_format);
            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);
#pragma GCC diagnostic pop
        });

    auto const window = create_surface([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_type(spec, old_type);
            mir_window_spec_set_width(spec, width);
            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                           mir_window_spec_set_pixel_format(spec, pixel_format);
                                           mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);
#pragma GCC diagnostic pop
        });

    latest_shell_surface()->add_observer(mt::fake_shared(surface_observer));

    EXPECT_CALL(surface_observer, attrib_changed(_, mir_window_attrib_type, new_type)).
        WillOnce(InvokeWithoutArgs([&] { change_observed(); }));

    change_surface(window, [&](MirWindowSpec* spec)
    {
        mir_window_spec_set_type(spec, new_type);
        mir_window_spec_set_parent(spec, parent);
    });
}

TEST_P(TargetNeedingParent, not_setting_parent_fails)
{
    auto const old_type = GetParam().from;
    auto const new_type = GetParam().to;

    auto const window = create_surface([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_type(spec, old_type);
            mir_window_spec_set_width(spec, width);
            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            mir_window_spec_set_pixel_format(spec, pixel_format);
            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);
#pragma GCC diagnostic pop
        });

    latest_shell_surface()->add_observer(mt::fake_shared(surface_observer));

    EXPECT_CALL(surface_observer, attrib_changed(_, mir_window_attrib_type, new_type)).
        Times(0);

    change_surface(window, [&](MirWindowSpec* spec)
        {
            mir_window_spec_set_type(spec, new_type);

            // Don't wait for a notification we don't expect
            // We'll wait for another change
            change_observed();
        });

    wait_for_arbitrary_change(window);
}

TEST_P(TargetMayHaveParent, setting_parent_succeeds)
{
    auto const old_type = GetParam().from;
    auto const new_type = GetParam().to;

    auto const parent = create_surface([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_type(spec, mir_window_type_normal);
            mir_window_spec_set_width(spec, width);
            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            mir_window_spec_set_pixel_format(spec, pixel_format);
            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);
#pragma GCC diagnostic pop
        });

    auto const window = create_surface([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_type(spec, old_type);
            mir_window_spec_set_width(spec, width);
            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                           mir_window_spec_set_pixel_format(spec, pixel_format);
                                           mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);
#pragma GCC diagnostic pop
        });

    latest_shell_surface()->add_observer(mt::fake_shared(surface_observer));

    EXPECT_CALL(surface_observer, attrib_changed(_, mir_window_attrib_type, new_type)).
        WillOnce(InvokeWithoutArgs([&] { change_observed(); }));

    change_surface(window, [&](MirWindowSpec* spec)
    {
        mir_window_spec_set_type(spec, new_type);
        mir_window_spec_set_parent(spec, parent);
    });
}

TEST_P(TargetMayHaveParent, not_setting_parent_succeeds)
{
    auto const old_type = GetParam().from;
    auto const new_type = GetParam().to;

    auto const parent = create_surface([&](MirWindowSpec* spec)
       {
           mir_window_spec_set_type(spec, mir_window_type_normal);
           mir_window_spec_set_width(spec, width);
           mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
           mir_window_spec_set_pixel_format(spec, pixel_format);
           mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);
#pragma GCC diagnostic pop
       });

    auto const window = create_surface([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_type(spec, old_type);
            mir_window_spec_set_width(spec, width);
            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            mir_window_spec_set_pixel_format(spec, pixel_format);
            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);
#pragma GCC diagnostic pop
        });

    latest_shell_surface()->add_observer(mt::fake_shared(surface_observer));

    EXPECT_CALL(surface_observer, attrib_changed(_, mir_window_attrib_type, new_type)).
    WillOnce(InvokeWithoutArgs([&] { change_observed(); }));


    change_surface(window, [&](MirWindowSpec* spec)
        {
            mir_window_spec_set_type(spec, new_type);
        });
}

INSTANTIATE_TEST_SUITE_P(SurfaceMorphing, TargetWithoutParent,
    Values(
        TypePair{mir_window_type_normal, mir_window_type_utility},
        TypePair{mir_window_type_utility, mir_window_type_normal},
        TypePair{mir_window_type_dialog, mir_window_type_utility},
        TypePair{mir_window_type_dialog, mir_window_type_normal}
    ));

INSTANTIATE_TEST_SUITE_P(SurfaceMorphing, TargetNeedingParent,
    Values(
        TypePair{mir_window_type_normal, mir_window_type_satellite},
        TypePair{mir_window_type_utility, mir_window_type_satellite},
        TypePair{mir_window_type_dialog, mir_window_type_satellite}
    ));

INSTANTIATE_TEST_SUITE_P(SurfaceMorphing, TargetMayHaveParent,
    Values(
        TypePair{mir_window_type_normal, mir_window_type_dialog},
        TypePair{mir_window_type_utility, mir_window_type_dialog}
    ));
