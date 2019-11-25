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

#include "mir/events/event_builders.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/scene/surface.h"

#include "mir/test/doubles/wrap_shell_to_track_latest_surface.h"
#include "mir_test_framework/connected_client_headless_server.h"
#include "mir_test_framework/visible_surface.h"
#include "mir/test/fake_shared.h"
#include "mir/test/signal.h"
#include "mir_toolkit/common.h"
#include "mir_toolkit/client_types.h"

#include <gmock/gmock.h>

namespace mev = mir::events;
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
    class MockSurfaceObserver : public ms::NullSurfaceObserver
    {
    public:
        MOCK_METHOD2(renamed, void(ms::Surface const*, char const*));
        MOCK_METHOD3(attrib_changed, void(ms::Surface const*, MirWindowAttrib attrib, int value));
        MOCK_METHOD2(content_resized_to, void(ms::Surface const*, Size const& size));
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
        mtf::VisibleSurface create_surface(Specifier const& specifier)
        {
            auto del = [] (MirWindowSpec* spec) { mir_window_spec_release(spec); };
            std::unique_ptr<MirWindowSpec, decltype(del)> spec(mir_create_window_spec(connection), del);
            specifier(spec.get());
            return mtf::VisibleSurface{spec.get()};
        }

        NiceMock<MockSurfaceObserver> surface_observer;

        void change_observed() { signal_change.raise(); }
        MirPixelFormat pixel_format{mir_pixel_format_invalid};
        static auto const width = 97;
        static auto const height= 101;
        MirInputDeviceId const device_id = MirInputDeviceId(7);
        std::chrono::nanoseconds const timestamp = std::chrono::nanoseconds(39);

        void generate_alt_click_at(Point const& click_position)
        {
            auto const modifiers = mir_input_event_modifier_alt;

            auto const x_axis_value = click_position.x.as_int();
            auto const y_axis_value = click_position.y.as_int();
            auto const hscroll_value = 0.0;
            auto const vscroll_value = 0.0;
            auto const action = mir_pointer_action_button_down;
            auto const relative_x_value = 0.0;
            auto const relative_y_value = 0.0;

            auto const click_event = mev::make_event(device_id, timestamp, cookie, modifiers,
                                                     action, mir_pointer_button_tertiary,
                                                     x_axis_value, y_axis_value,
                                                     hscroll_value, vscroll_value,
                                                     relative_x_value, relative_y_value);

            server.the_shell()->handle(*click_event);
        }

        void generate_alt_move_to(Point const& drag_position)
        {
            auto const modifiers = mir_input_event_modifier_alt;

            auto const x_axis_value = drag_position.x.as_int();
            auto const y_axis_value = drag_position.y.as_int();
            auto const hscroll_value = 0.0;
            auto const vscroll_value = 0.0;
            auto const action = mir_pointer_action_motion;
            auto const relative_x_value = 0.0;
            auto const relative_y_value = 0.0;

            auto const drag_event = mev::make_event(device_id, timestamp, cookie, modifiers,
                                                    action, mir_pointer_button_tertiary,
                                                    x_axis_value, y_axis_value,
                                                    hscroll_value, vscroll_value,
                                                    relative_x_value, relative_y_value);

            server.the_shell()->handle(*drag_event);
        }

        void wait_for_arbitrary_change(MirWindow* surface)
        {
            auto const new_title = __PRETTY_FUNCTION__;

            EXPECT_CALL(surface_observer, renamed(_, StrEq(new_title))).
                    WillOnce(InvokeWithoutArgs([&]{ change_observed(); }));

            signal_change.reset();

            auto const spec = mir_create_window_spec(connection);

            mir_window_spec_set_name(spec, new_title);

            mir_window_apply_spec(surface, spec);
            mir_window_spec_release(spec);
            signal_change.wait_for(1s);
        }

    private:
        std::shared_ptr<mtd::WrapShellToTrackLatestSurface> shell;
        mt::Signal signal_change;
        std::vector<uint8_t> cookie;

        void init_pixel_format()
        {
            unsigned int valid_formats{0};
            mir_connection_get_available_surface_formats(connection, &pixel_format, 1, &valid_formats);
        }
    };

    struct SurfaceSpecificationCase : SurfaceSpecification, ::testing::WithParamInterface<MirWindowType> {};
    using SurfaceWithoutParent = SurfaceSpecificationCase;
    using SurfaceNeedingParent = SurfaceSpecificationCase;
    using SurfaceMayHaveParent = SurfaceSpecificationCase;

    MATCHER(IsValidSurface, "")
    {
        if (arg == nullptr) return false;

        if (!mir_window_is_valid(arg)) return false;

        return true;
    }

    MATCHER_P(WidthEq, value, "")
    {
        return Width(value) == arg.width;
    }

    MATCHER_P(HeightEq, value, "")
    {
        return Height(value) == arg.height;
    }
}

TEST_F(SurfaceSpecification, surface_spec_min_width_is_respected)
{
    auto const min_width = 17;

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, mir_window_type_normal);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                            mir_window_spec_set_min_width(spec, min_width);
                                        });

    auto const shell_surface = latest_shell_surface();
    shell_surface->add_observer(mt::fake_shared(surface_observer));

    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    EXPECT_CALL(surface_observer, content_resized_to(_, WidthEq(min_width)));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(shell_surface->top_left());
}

TEST_F(SurfaceSpecification, surface_spec_min_height_is_respected)
{
    auto const min_height = 19;

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, mir_window_type_normal);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                            mir_window_spec_set_min_height(spec, min_height);
                                        });

    auto const shell_surface = latest_shell_surface();
    shell_surface->add_observer(mt::fake_shared(surface_observer));

    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    EXPECT_CALL(surface_observer, content_resized_to(_, HeightEq(min_height)));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(shell_surface->top_left());
}

TEST_F(SurfaceSpecification, surface_spec_max_width_is_respected)
{
    auto const max_width = 23;

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, mir_window_type_normal);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                            mir_window_spec_set_max_width(spec, max_width);
                                        });

    auto const shell_surface = latest_shell_surface();
    shell_surface->add_observer(mt::fake_shared(surface_observer));

    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    EXPECT_CALL(surface_observer, content_resized_to(_, WidthEq(max_width)));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_right + DeltaX(max_width));
}

TEST_F(SurfaceSpecification, surface_spec_max_height_is_respected)
{
    auto const max_height = 29;

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, mir_window_type_normal);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                            mir_window_spec_set_max_height(spec, max_height);
                                        });

    auto const shell_surface = latest_shell_surface();
    shell_surface->add_observer(mt::fake_shared(surface_observer));

    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    EXPECT_CALL(surface_observer, content_resized_to(_, HeightEq(max_height)));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_right + DeltaY(max_height));
}

TEST_F(SurfaceSpecification, surface_spec_width_inc_is_respected)
{
    auto const width_inc = 13;

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, mir_window_type_normal);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                            mir_window_spec_set_width_increment(spec, width_inc);
                                        });

    auto const shell_surface = latest_shell_surface();
    shell_surface->add_observer(mt::fake_shared(surface_observer));

    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    Size actual;
    EXPECT_CALL(surface_observer, content_resized_to(_, _)).WillOnce(SaveArg<1>(&actual));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_right + DeltaX(16));

    EXPECT_TRUE(actual.width.as_int() % width_inc == 0);
}

TEST_F(SurfaceSpecification, surface_spec_with_min_width_and_width_inc_is_respected)
{
    auto const width_inc = 13;
    auto const min_width = 7;

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, mir_window_type_normal);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                            mir_window_spec_set_width_increment(spec, width_inc);
                                            mir_window_spec_set_min_width(spec, min_width);
                                        });

    auto const shell_surface = latest_shell_surface();
    shell_surface->add_observer(mt::fake_shared(surface_observer));

    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    Size actual;
    EXPECT_CALL(surface_observer, content_resized_to(_, _)).WillOnce(SaveArg<1>(&actual));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_right + DeltaX(16));

    EXPECT_TRUE((actual.width.as_int() - min_width) % width_inc == 0);
}

TEST_F(SurfaceSpecification, surface_spec_height_inc_is_respected)
{
    auto const height_inc = 13;

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, mir_window_type_normal);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                            mir_window_spec_set_height_increment(spec, height_inc);
                                        });

    auto const shell_surface = latest_shell_surface();
    shell_surface->add_observer(mt::fake_shared(surface_observer));

    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    Size actual;
    EXPECT_CALL(surface_observer, content_resized_to(_, _)).WillOnce(SaveArg<1>(&actual));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_right + DeltaY(16));

    EXPECT_TRUE(actual.height.as_int() % height_inc == 0);
}

TEST_F(SurfaceSpecification, surface_spec_with_min_height_and_height_inc_is_respected)
{
    auto const height_inc = 13;
    auto const min_height = 7;

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, mir_window_type_normal);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                            mir_window_spec_set_height_increment(spec, height_inc);
                                            mir_window_spec_set_min_height(spec, min_height);
                                        });

    auto const shell_surface = latest_shell_surface();
    shell_surface->add_observer(mt::fake_shared(surface_observer));

    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    Size actual;
    EXPECT_CALL(surface_observer, content_resized_to(_, _)).WillOnce(SaveArg<1>(&actual));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_right + DeltaY(16));

    EXPECT_TRUE((actual.height.as_int() - min_height) % height_inc == 0);
}

TEST_F(SurfaceSpecification, surface_spec_with_min_aspect_ratio_is_respected)
{
    auto const aspect_width = 11;
    auto const aspect_height = 7;

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, mir_window_type_normal);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                            mir_window_spec_set_min_aspect_ratio(spec, aspect_width, aspect_height);
                                        });

    auto const shell_surface = latest_shell_surface();
    shell_surface->add_observer(mt::fake_shared(surface_observer));

    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};
    auto const bottom_left = shell_surface->input_bounds().bottom_left() + Displacement{1,-1};

    Size actual;
    EXPECT_CALL(surface_observer, content_resized_to(_, _)).WillOnce(SaveArg<1>(&actual));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_left);

    EXPECT_THAT(float(actual.width.as_int())/actual.height.as_int(), Ge(float(aspect_width)/aspect_height));
}

TEST_F(SurfaceSpecification, surface_spec_with_max_aspect_ratio_is_respected)
{
    auto const aspect_width = 7;
    auto const aspect_height = 11;

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, mir_window_type_normal);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                            mir_window_spec_set_max_aspect_ratio(spec, aspect_width, aspect_height);
                                        });

    auto const shell_surface = latest_shell_surface();
    shell_surface->add_observer(mt::fake_shared(surface_observer));

    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};
    auto const top_right = shell_surface->input_bounds().top_right() - Displacement{1,-1};

    Size actual;
    EXPECT_CALL(surface_observer, content_resized_to(_, _)).WillOnce(SaveArg<1>(&actual));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(top_right);

    EXPECT_THAT(float(actual.width.as_int())/actual.height.as_int(), Le(float(aspect_width)/aspect_height));
}

TEST_F(SurfaceSpecification, surface_spec_with_fixed_aspect_ratio_and_size_range_is_respected)
{
    auto const aspect_width = 11;
    auto const aspect_height = 7;
    auto const min_width = 10*aspect_width;
    auto const min_height = 10*aspect_height;
    auto const max_width = 20*aspect_width;
    auto const max_height = 20*aspect_height;
    auto const width_inc = 11;
    auto const height_inc = 7;
    auto const expected_aspect_ratio = FloatEq(float(aspect_width)/aspect_height);

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, mir_window_type_normal);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop

                                            mir_window_spec_set_min_aspect_ratio(spec, aspect_width, aspect_height);
                                            mir_window_spec_set_max_aspect_ratio(spec, aspect_width, aspect_height);

                                            mir_window_spec_set_min_height(spec, min_height);
                                            mir_window_spec_set_min_width(spec, min_width);

                                            mir_window_spec_set_max_height(spec, max_height);
                                            mir_window_spec_set_max_width(spec, max_width);

                                            mir_window_spec_set_width_increment(spec, width_inc);
                                            mir_window_spec_set_height_increment(spec, height_inc);

                                            mir_window_spec_set_height(spec, min_height);
                                            mir_window_spec_set_width(spec, min_width);
                                        });

    auto const shell_surface = latest_shell_surface();
    shell_surface->add_observer(mt::fake_shared(surface_observer));

    Size actual;
    EXPECT_CALL(surface_observer, content_resized_to(_, _)).Times(AnyNumber()).WillRepeatedly(SaveArg<1>(&actual));

    for (int delta = 1; delta != 20; ++delta)
    {
        auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

        // Introduce small variation around "accurate" resize motion
        auto const jitter = Displacement{delta%2 ? +2 : -2, (delta/2)%2 ? +2 : -2};
        auto const motion = Displacement{width_inc, height_inc} + jitter;

        generate_alt_click_at(bottom_right);
        generate_alt_move_to(bottom_right + motion);

        Size const expected_size{
                std::min(max_width,  min_width  + delta*width_inc),
                std::min(max_height, min_height + delta*height_inc)};

        EXPECT_THAT(static_cast<float>(actual.width.as_int())/actual.height.as_int(), expected_aspect_ratio);
        EXPECT_THAT(actual, Eq(expected_size));
    };
}

TEST_P(SurfaceWithoutParent, not_setting_parent_succeeds)
{
    auto const type = GetParam();

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, type);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                        });

    EXPECT_THAT(surface, IsValidSurface());
}

TEST_P(SurfaceWithoutParent, setting_parent_fails)
{
    auto const type = GetParam();

    auto const parent = create_surface([&](MirWindowSpec* spec)
                                       {
                                           mir_window_spec_set_type(spec, mir_window_type_normal);
                                           mir_window_spec_set_width(spec, width);
                                           mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                           mir_window_spec_set_pixel_format(spec, pixel_format);
                                           mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                       });

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, type);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                            mir_window_spec_set_parent(spec, parent);
                                        });

    EXPECT_THAT(surface, Not(IsValidSurface()));
}

TEST_P(SurfaceNeedingParent, setting_parent_succeeds)
{
    auto const type = GetParam();

    auto const parent = create_surface([&](MirWindowSpec* spec)
                                       {
                                           mir_window_spec_set_type(spec, mir_window_type_normal);
                                           mir_window_spec_set_width(spec, width);
                                           mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                           mir_window_spec_set_pixel_format(spec, pixel_format);
                                           mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                       });

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, type);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                            mir_window_spec_set_parent(spec, parent);
                                        });

    EXPECT_THAT(surface, IsValidSurface());
}

TEST_P(SurfaceNeedingParent, not_setting_parent_fails)
{
    auto const type = GetParam();

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, type);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                        });

    EXPECT_THAT(surface, Not(IsValidSurface()));
}

TEST_P(SurfaceMayHaveParent, setting_parent_succeeds)
{
    auto const type = GetParam();

    auto const parent = create_surface([&](MirWindowSpec* spec)
                                       {
                                           mir_window_spec_set_type(spec, mir_window_type_normal);
                                           mir_window_spec_set_width(spec, width);
                                           mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                           mir_window_spec_set_pixel_format(spec, pixel_format);
                                           mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                       });

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, type);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
                                            mir_window_spec_set_parent(spec, parent);
                                        });

    EXPECT_THAT(surface, IsValidSurface());
}

TEST_P(SurfaceMayHaveParent, not_setting_parent_succeeds)
{
    auto const type = GetParam();

    auto const surface = create_surface([&](MirWindowSpec* spec)
                                        {
                                            mir_window_spec_set_type(spec, type);
                                            mir_window_spec_set_width(spec, width);
                                            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                                            mir_window_spec_set_pixel_format(spec, pixel_format);
                                            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop

                                        });

    EXPECT_THAT(surface, IsValidSurface());
}

INSTANTIATE_TEST_CASE_P(SurfaceSpecification, SurfaceWithoutParent,
                        Values(mir_window_type_utility, mir_window_type_normal));

INSTANTIATE_TEST_CASE_P(SurfaceSpecification, SurfaceNeedingParent,
                        Values(mir_window_type_satellite, mir_window_type_gloss, mir_window_type_tip));

INSTANTIATE_TEST_CASE_P(SurfaceSpecification, SurfaceMayHaveParent,
                        Values(mir_window_type_dialog, mir_window_type_freestyle));
