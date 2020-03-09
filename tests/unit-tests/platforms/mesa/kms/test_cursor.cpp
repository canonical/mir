/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/platforms/mesa/server/kms/cursor.h"
#include "src/platforms/mesa/server/kms/kms_output.h"
#include "src/platforms/mesa/server/kms/kms_output_container.h"
#include "src/platforms/mesa/server/kms/kms_display_configuration.h"

#include "mir/graphics/cursor_image.h"

#include <xf86drm.h>

#include "mir/test/doubles/mock_gbm.h"
#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/fake_shared.h"
#include "mir_test_framework/temporary_environment_value.h"
#include "mock_kms_output.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <unordered_map>
#include <algorithm>

#include <string.h>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;
using mt::MockKMSOutput;
using namespace ::testing;

namespace
{

struct StubKMSOutputContainer : public mgm::KMSOutputContainer
{
    StubKMSOutputContainer()
        : outputs{
            std::make_shared<testing::NiceMock<MockKMSOutput>>(),
            std::make_shared<testing::NiceMock<MockKMSOutput>>(),
            std::make_shared<testing::NiceMock<MockKMSOutput>>()}
    {
        // These need to be established before Cursor construction:
        for (auto& output : outputs)
        {
            auto& out = *output;
            ON_CALL(out, has_cursor())
                .WillByDefault(Return(true));
            ON_CALL(out, set_cursor(_))
                .WillByDefault(Return(true));
            ON_CALL(out, clear_cursor())
                .WillByDefault(Return(true));
        }
    }

    void for_each_output(std::function<void(std::shared_ptr<mgm::KMSOutput> const&)> functor) const
    {
        for (auto const& output : outputs)
            functor(output);
    }

    void verify_and_clear_expectations()
    {
        for (auto const& output : outputs)
            ::testing::Mock::VerifyAndClearExpectations(output.get());
    }

    void update_from_hardware_state()
    {
    }

    std::vector<std::shared_ptr<testing::NiceMock<MockKMSOutput>>> outputs;
};

struct StubKMSDisplayConfiguration : public mgm::KMSDisplayConfiguration
{
    StubKMSDisplayConfiguration(mgm::KMSOutputContainer& container)
        : mgm::KMSDisplayConfiguration(),
          container{container},
          stub_config{
            {{
                mg::DisplayConfigurationOutputId{0},
                mg::DisplayConfigurationCardId{},
                mg::DisplayConfigurationOutputType::vga,
                {},
                {
                    {geom::Size{10, 20}, 59.9},
                    {geom::Size{200, 100}, 59.9},
                },
                1,
                geom::Size{324, 642},
                true,
                true,
                geom::Point{0, 0},
                1,
                mir_pixel_format_invalid,
                mir_power_mode_on,
                mir_orientation_normal,
                1.0f,
                mir_form_factor_monitor,
                mir_subpixel_arrangement_unknown,
                {},
                mir_output_gamma_unsupported,
                {},
                {}
            },
            {
                mg::DisplayConfigurationOutputId{1},
                mg::DisplayConfigurationCardId{},
                mg::DisplayConfigurationOutputType::vga,
                {},
                {
                    {geom::Size{200, 200}, 59.9},
                    {geom::Size{100, 200}, 59.9},
                },
                0,
                geom::Size{566, 111},
                true,
                true,
                geom::Point{100, 50},
                0,
                mir_pixel_format_invalid,
                mir_power_mode_on,
                mir_orientation_normal,
                1.0f,
                mir_form_factor_monitor,
                mir_subpixel_arrangement_unknown,
                {},
                mir_output_gamma_unsupported,
                {},
                {}
            },
            {
                mg::DisplayConfigurationOutputId{2},
                mg::DisplayConfigurationCardId{},
                mg::DisplayConfigurationOutputType::vga,
                {},
                {
                    {geom::Size{800, 200}, 59.9},
                    {geom::Size{100, 200}, 59.9},
                },
                0,
                geom::Size{800, 200},
                true,
                true,
                geom::Point{666, 0},
                0,
                mir_pixel_format_invalid,
                mir_power_mode_on,
                mir_orientation_right,
                1.0f,
                mir_form_factor_monitor,
                mir_subpixel_arrangement_unknown,
                {},
                mir_output_gamma_unsupported,
                {},
                {}
            }}}
    {
        update();
    }

    void for_each_card(std::function<void(mg::DisplayConfigurationCard const&)> f) const override
    {
        stub_config.for_each_card(f);
    }

    void for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const override
    {
        stub_config.for_each_output(f);
    }

    void for_each_output(std::function<void(mg::UserDisplayConfigurationOutput&)> f) override
    {
        stub_config.for_each_output(f);
    }

    std::unique_ptr<DisplayConfiguration> clone() const override
    {
        return stub_config.clone();
    }

    bool valid() const override
    {
        return stub_config.valid();
    }

    std::shared_ptr<mgm::KMSOutput> get_output_for(mg::DisplayConfigurationOutputId id) const override
    {
        return outputs[id.as_value()];
    }

    size_t get_kms_mode_index(mg::DisplayConfigurationOutputId, size_t conf_mode_index) const override
    {
        return conf_mode_index;
    }

    void update() override
    {
        container.for_each_output(
            [this](auto const& output)
            {
                outputs.push_back(output);
            });
    }

    void set_orentation_of_output(mg::DisplayConfigurationOutputId id, MirOrientation orientation)
    {
        stub_config.for_each_output(
            [id,orientation] (mg::UserDisplayConfigurationOutput const& output)
            {
                if (output.id == id)
                    output.orientation = orientation;
            });
    }

    mgm::KMSOutputContainer& container;
    mtd::StubDisplayConfig stub_config;
    std::vector<std::shared_ptr<mgm::KMSOutput>> outputs;
};

struct StubCurrentConfiguration : public mgm::CurrentConfiguration
{
    StubCurrentConfiguration(mgm::KMSOutputContainer& container)
        : conf(container)
    {
    }

    void with_current_configuration_do(
        std::function<void(mgm::KMSDisplayConfiguration const&)> const& exec)
    {
        exec(conf);
    }

    StubKMSDisplayConfiguration conf;
};

struct StubCursorImage : public mg::CursorImage
{
    void const* as_argb_8888() const
    {
        return image_data;
    }
    geom::Size size() const
    {
        return geom::Size{geom::Width{64}, geom::Height{64}};
    }
    geom::Displacement hotspot() const
    {
        return geom::Displacement{0, 0};
    }

    static void const* image_data;
};

char const stub_data[128*128*4] = { 0 };
void const* StubCursorImage::image_data = stub_data;

// Those new cap flags are currently only available in drm/drm.h but not in
// libdrm/drm.h nor in xf86drm.h. Additionally drm/drm.h is current c++ unfriendly
// So until those headers get cleaned up we duplicate those definitions.
#ifndef DRM_CAP_CURSOR_WIDTH
#define DRM_CAP_CURSOR_WIDTH            0x8
#define DRM_CAP_CURSOR_HEIGHT           0x9
#endif

struct MesaCursorTest : ::testing::Test
{
    struct MockGBM : testing::NiceMock<mtd::MockGBM>
    {
        MockGBM()
        {
            using namespace ::testing;
            const uint32_t default_cursor_size = 64;
            ON_CALL(*this, gbm_bo_get_width(_))
                .WillByDefault(Return(default_cursor_size));
            ON_CALL(*this, gbm_bo_get_height(_))
                .WillByDefault(Return(default_cursor_size));

        }
    } mock_gbm;

    size_t const cursor_side{64};
    MesaCursorTest()
        : cursor{output_container,
            mt::fake_shared(current_configuration)}
    {
        using namespace ::testing;
        ON_CALL(mock_drm, drmGetCap(_, DRM_CAP_CURSOR_WIDTH, _))
            .WillByDefault(Invoke([this](int , uint64_t , uint64_t *value)
                                  {
                                      *value = cursor_side;
                                      return 0;
                                  }));
        ON_CALL(mock_drm, drmGetCap(_, DRM_CAP_CURSOR_HEIGHT, _))
            .WillByDefault(Invoke([this](int , uint64_t , uint64_t *value)
                                  {
                                      *value = cursor_side;
                                      return 0;
                                  }));

    }

    testing::NiceMock<mtd::MockDRM> mock_drm;
    StubCursorImage stub_image;
    StubKMSOutputContainer output_container;
    StubCurrentConfiguration current_configuration{output_container};
    mgm::Cursor cursor;
};

struct SinglePixelCursorImage : public StubCursorImage
{
    geom::Size size() const
    {
        return small_cursor_size;
    }
    void const* as_argb_8888() const
    {
        static uint32_t const pixel = 0xffffffff;
        return &pixel;
    }

    size_t const cursor_side{1};
    geom::Size const small_cursor_size{cursor_side, cursor_side};
};

}

TEST_F(MesaCursorTest, creates_cursor_bo_image)
{
    EXPECT_CALL(mock_gbm, gbm_bo_create(mock_gbm.fake_gbm.device,
                                        cursor_side, cursor_side,
                                        GBM_FORMAT_ARGB8888,
                                        GBM_BO_USE_CURSOR | GBM_BO_USE_WRITE));

    mgm::Cursor cursor_tmp{output_container,
        std::make_shared<StubCurrentConfiguration>(output_container)};
}

TEST_F(MesaCursorTest, queries_received_cursor_size)
{
    using namespace ::testing;

    // Our standard mock DRM has 2 GPU devices, and we should construct a cursor on both.
    EXPECT_CALL(mock_gbm, gbm_bo_get_width(_)).Times(2);
    EXPECT_CALL(mock_gbm, gbm_bo_get_height(_)).Times(2);

    mgm::Cursor cursor_tmp{output_container,
        std::make_shared<StubCurrentConfiguration>(output_container)};
}

TEST_F(MesaCursorTest, respects_drm_cap_cursor)
{
    auto const drm_buffer_size = 255;
    ON_CALL(mock_drm, drmGetCap(_, DRM_CAP_CURSOR_WIDTH, _))
        .WillByDefault(Invoke([](int , uint64_t , uint64_t *value) { *value = drm_buffer_size; return 0; }));

    ON_CALL(mock_drm, drmGetCap(_, DRM_CAP_CURSOR_HEIGHT, _))
        .WillByDefault(Invoke([](int , uint64_t , uint64_t *value) { *value = drm_buffer_size; return 0; }));

    EXPECT_CALL(mock_gbm, gbm_bo_create(_, drm_buffer_size, drm_buffer_size, _, _));

    mgm::Cursor cursor_tmp{output_container,
                           std::make_shared<StubCurrentConfiguration>(output_container)};
}

TEST_F(MesaCursorTest, can_force_64x64_cursor)
{
    auto const drm_buffer_size = 255;
    ON_CALL(mock_drm, drmGetCap(_, DRM_CAP_CURSOR_WIDTH, _))
        .WillByDefault(Invoke([](int , uint64_t , uint64_t *value) { *value = drm_buffer_size; return 0; }));

    ON_CALL(mock_drm, drmGetCap(_, DRM_CAP_CURSOR_HEIGHT, _))
        .WillByDefault(Invoke([](int , uint64_t , uint64_t *value) { *value = drm_buffer_size; return 0; }));

    mir_test_framework::TemporaryEnvironmentValue mir_drm_cursor_64x64{"MIR_DRM_CURSOR_64x64", "on"};

    EXPECT_CALL(mock_gbm, gbm_bo_create(_, 64, 64, _, _));

    mgm::Cursor cursor_tmp{output_container,
                           std::make_shared<StubCurrentConfiguration>(output_container)};
}

TEST_F(MesaCursorTest, show_cursor_writes_to_bo)
{
    using namespace testing;

    StubCursorImage image;
    geom::Size const cursor_size{cursor_side, cursor_side};
    size_t const cursor_size_bytes{cursor_side * cursor_side * sizeof(uint32_t)};

    EXPECT_CALL(mock_gbm, gbm_bo_write(mock_gbm.fake_gbm.bo, NotNull(), cursor_size_bytes));

    cursor.show(image);
}

// When we upload our 1x1 cursor we should upload a single white pixel and then transparency filling a 64x64 buffer.
MATCHER_P(ContainsASingleWhitePixel, buffersize, "")
{
    auto pixels = static_cast<uint32_t const*>(arg);
    if (pixels[0] != 0xffffffff)
        return false;
    for (auto i = decltype(buffersize){1}; i < buffersize; i++)
    {
        if (pixels[i] != 0x0)
            return false;
    }
    return true;
}

TEST_F(MesaCursorTest, show_cursor_pads_missing_data)
{
    using namespace testing;
    // We expect a full 64x64 pixel write as we will pad missing data with transparency.
    size_t const height = 64;
    size_t const width = 64;
    size_t const stride = width * 4;
    size_t const buffer_size_bytes{height * stride};
    ON_CALL(mock_gbm, gbm_bo_get_stride(_))
        .WillByDefault(Return(stride));
    EXPECT_CALL(mock_gbm, gbm_bo_write(mock_gbm.fake_gbm.bo, ContainsASingleWhitePixel(width*height), buffer_size_bytes));

    cursor.show(SinglePixelCursorImage());
}

TEST_F(MesaCursorTest, pads_missing_data_when_buffer_size_differs)
{
    using namespace ::testing;

    size_t const height = 128;
    size_t const width = 128;
    size_t const stride = width * 4;
    size_t const buffer_size_bytes{height * stride};

    ON_CALL(mock_gbm, gbm_bo_get_width(mock_gbm.fake_gbm.bo))
        .WillByDefault(Return(width));
    ON_CALL(mock_gbm, gbm_bo_get_height(mock_gbm.fake_gbm.bo))
        .WillByDefault(Return(height));
    ON_CALL(mock_gbm, gbm_bo_get_stride(mock_gbm.fake_gbm.bo))
        .WillByDefault(Return(stride));

    EXPECT_CALL(mock_gbm, gbm_bo_write(mock_gbm.fake_gbm.bo, ContainsASingleWhitePixel(width*height), buffer_size_bytes));

    mgm::Cursor cursor_tmp{output_container,
        std::make_shared<StubCurrentConfiguration>(output_container)};
    cursor_tmp.show(SinglePixelCursorImage());
}

TEST_F(MesaCursorTest, does_not_throw_when_images_are_too_large)
{
    using namespace testing;

    struct LargeCursorImage : public StubCursorImage
    {
        geom::Size size() const
        {
            return large_cursor_size;
        }
        size_t const cursor_side{128};
        geom::Size const large_cursor_size{cursor_side, cursor_side};
    };

    cursor.show(LargeCursorImage());
}

TEST_F(MesaCursorTest, clears_cursor_state_on_construction)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[0], clear_cursor());
    EXPECT_CALL(*output_container.outputs[1], clear_cursor());
    EXPECT_CALL(*output_container.outputs[2], clear_cursor());

    /* No checking of existing cursor state */
    EXPECT_CALL(*output_container.outputs[0], has_cursor()).Times(0);
    EXPECT_CALL(*output_container.outputs[1], has_cursor()).Times(0);
    EXPECT_CALL(*output_container.outputs[2], has_cursor()).Times(0);

    mgm::Cursor cursor_tmp{output_container,
       std::make_shared<StubCurrentConfiguration>(output_container)};

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, construction_fails_if_initial_set_fails)
{
    using namespace testing;

    ON_CALL(*output_container.outputs[0], clear_cursor())
        .WillByDefault(Return(false));
    ON_CALL(*output_container.outputs[0], set_cursor(_))
        .WillByDefault(Return(false));
    ON_CALL(*output_container.outputs[0], has_cursor())
        .WillByDefault(Return(false));

    EXPECT_THROW(
        mgm::Cursor cursor_tmp(output_container,
           std::make_shared<StubCurrentConfiguration>(output_container));
    , std::runtime_error);
}

TEST_F(MesaCursorTest, move_to_sets_clears_cursor_if_needed)
{
    using namespace testing;

    cursor.show(stub_image);

    EXPECT_CALL(*output_container.outputs[0], has_cursor())
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    EXPECT_CALL(*output_container.outputs[0], set_cursor(_));

    EXPECT_CALL(*output_container.outputs[1], has_cursor())
        .WillOnce(Return(true));
    EXPECT_CALL(*output_container.outputs[1], clear_cursor());

    cursor.move_to({10, 10});

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, move_to_doesnt_set_clear_cursor_if_not_needed)
{
    using namespace testing;

    cursor.show(stub_image);

    EXPECT_CALL(*output_container.outputs[0], has_cursor())
        .WillOnce(Return(true));
    EXPECT_CALL(*output_container.outputs[0], set_cursor(_))
        .Times(0);

    EXPECT_CALL(*output_container.outputs[1], has_cursor())
        .WillOnce(Return(false));
    EXPECT_CALL(*output_container.outputs[1], clear_cursor())
        .Times(0);

    cursor.move_to({10, 10});

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, move_to_moves_cursor_to_right_output)
{
    using namespace testing;

    cursor.show(stub_image);

    EXPECT_CALL(*output_container.outputs[0], move_cursor(geom::Point{10,10}));
    EXPECT_CALL(*output_container.outputs[1], move_cursor(_))
        .Times(0);

    cursor.move_to({10, 10});

    output_container.verify_and_clear_expectations();

    EXPECT_CALL(*output_container.outputs[0], move_cursor(_))
        .Times(0);
    EXPECT_CALL(*output_container.outputs[1], move_cursor(geom::Point{50,100}));

    cursor.move_to({150, 150});

    output_container.verify_and_clear_expectations();

    EXPECT_CALL(*output_container.outputs[0], move_cursor(geom::Point{150,75}));
    EXPECT_CALL(*output_container.outputs[1], move_cursor(geom::Point{50,25}));

    cursor.move_to({150, 75});

    output_container.verify_and_clear_expectations();

    EXPECT_CALL(*output_container.outputs[0], move_cursor(_))
        .Times(0);
    EXPECT_CALL(*output_container.outputs[1], move_cursor(_))
        .Times(0);

    cursor.move_to({-1, -1});

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, moves_properly_to_and_inside_left_rotated_output)
{
    using namespace testing;

    cursor.show(stub_image);

    current_configuration.conf.set_orentation_of_output(mg::DisplayConfigurationOutputId{2}, mir_orientation_left);

    InSequence seq;
    EXPECT_CALL(*output_container.outputs[2], move_cursor(geom::Point{112, 36}));
    EXPECT_CALL(*output_container.outputs[2], move_cursor(geom::Point{150, 32}));

    cursor.move_to({766, 112});
    cursor.move_to({770, 150});

    output_container.verify_and_clear_expectations();
}


TEST_F(MesaCursorTest, moves_properly_to_and_inside_right_rotated_output)
{
    using namespace testing;

    cursor.show(stub_image);

    current_configuration.conf.set_orentation_of_output(mg::DisplayConfigurationOutputId{2}, mir_orientation_right);

    InSequence seq;
    EXPECT_CALL(*output_container.outputs[2], move_cursor(geom::Point{624, 100}));
    EXPECT_CALL(*output_container.outputs[2], move_cursor(geom::Point{586, 104}));

    cursor.move_to({766, 112});
    cursor.move_to({770, 150});

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, moves_properly_to_and_inside_inverted_output)
{
    using namespace testing;

    cursor.show(stub_image);

    current_configuration.conf.set_orentation_of_output(mg::DisplayConfigurationOutputId{2}, mir_orientation_inverted);

    InSequence seq;
    EXPECT_CALL(*output_container.outputs[2], move_cursor(geom::Point{636, 24}));
    EXPECT_CALL(*output_container.outputs[2], move_cursor(geom::Point{632,-14}));

    cursor.move_to({766, 112});
    cursor.move_to({770, 150});

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, hides_cursor_in_all_outputs)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[0], clear_cursor());
    EXPECT_CALL(*output_container.outputs[1], clear_cursor());
    EXPECT_CALL(*output_container.outputs[2], clear_cursor());

    cursor.hide();

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, hidden_cursor_is_not_shown_on_display_when_moved)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[0], clear_cursor());
    EXPECT_CALL(*output_container.outputs[1], clear_cursor());
    EXPECT_CALL(*output_container.outputs[2], clear_cursor());
    EXPECT_CALL(*output_container.outputs[0], move_cursor(_)).Times(0);
    EXPECT_CALL(*output_container.outputs[1], move_cursor(_)).Times(0);
    EXPECT_CALL(*output_container.outputs[2], move_cursor(_)).Times(0);

    cursor.hide();
    cursor.move_to({17, 29});

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, clears_cursor_on_exit)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[0], clear_cursor());
    EXPECT_CALL(*output_container.outputs[1], clear_cursor());
    EXPECT_CALL(*output_container.outputs[2], clear_cursor());
}

TEST_F(MesaCursorTest, cursor_is_shown_at_correct_location_after_suspend_resume)
{
    using namespace testing;

    cursor.show(stub_image);

    EXPECT_CALL(*output_container.outputs[0], move_cursor(geom::Point{150,75}));
    EXPECT_CALL(*output_container.outputs[1], move_cursor(geom::Point{50,25}));
    EXPECT_CALL(*output_container.outputs[0], clear_cursor());
    EXPECT_CALL(*output_container.outputs[1], clear_cursor());
    EXPECT_CALL(*output_container.outputs[2], has_cursor())
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*output_container.outputs[2], clear_cursor());

    cursor.move_to({150, 75});
    cursor.suspend();

    output_container.verify_and_clear_expectations();

    EXPECT_CALL(*output_container.outputs[0], set_cursor(_));
    EXPECT_CALL(*output_container.outputs[1], set_cursor(_));
    EXPECT_CALL(*output_container.outputs[0], move_cursor(geom::Point{150,75}));
    EXPECT_CALL(*output_container.outputs[1], move_cursor(geom::Point{50,25}));

    cursor.resume();
    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, hidden_cursor_is_not_shown_after_suspend_resume)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[0], clear_cursor()).Times(AnyNumber());
    EXPECT_CALL(*output_container.outputs[1], clear_cursor()).Times(AnyNumber());
    EXPECT_CALL(*output_container.outputs[2], clear_cursor()).Times(AnyNumber());

    cursor.hide();
    cursor.suspend();

    output_container.verify_and_clear_expectations();

    EXPECT_CALL(*output_container.outputs[0], set_cursor(_)).Times(0);
    EXPECT_CALL(*output_container.outputs[1], set_cursor(_)).Times(0);
    EXPECT_CALL(*output_container.outputs[2], set_cursor(_)).Times(0);

    cursor.resume();
    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, show_with_param_places_cursor_on_output)
{
    EXPECT_CALL(*output_container.outputs[0], clear_cursor());
    cursor.hide();

    output_container.verify_and_clear_expectations();

    EXPECT_CALL(*output_container.outputs[0], set_cursor(_));
    cursor.show(stub_image);
}

TEST_F(MesaCursorTest, show_cursor_sets_cursor_with_hotspot)
{
    using namespace testing;

    cursor.show(stub_image); // ensures initial_cursor_location

    static geom::Displacement hotspot_displacement{10, 10};

    static geom::Point const
        initial_cursor_location = {0, 0},
        cursor_location_1 = {20, 20},
        cursor_location_2 = {40, 12};
    static geom::Point const initial_buffer_location = initial_cursor_location - hotspot_displacement;
    static geom::Point const expected_buffer_location_1 = cursor_location_1 - hotspot_displacement;
    static geom::Point const expected_buffer_location_2 = cursor_location_2 - hotspot_displacement;

    struct HotspotCursor : public StubCursorImage
    {
        geom::Displacement hotspot() const override
        {
            return hotspot_displacement;
        }
    };


    EXPECT_CALL(mock_gbm, gbm_bo_write(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(*output_container.outputs[0], set_cursor(_)).Times(AnyNumber());

    // When we set the image with the hotspot, first we should see the cursor move from its initial
    // location, to account for the displacement. Further movement should be offset by the hotspot.
    {
        InSequence seq;
        EXPECT_CALL(*output_container.outputs[0], move_cursor(initial_buffer_location));
        EXPECT_CALL(*output_container.outputs[0], move_cursor(expected_buffer_location_1));
        EXPECT_CALL(*output_container.outputs[0], move_cursor(expected_buffer_location_2));
    }

    cursor.show(HotspotCursor());
    cursor.move_to(cursor_location_1);
    cursor.move_to(cursor_location_2);
}

