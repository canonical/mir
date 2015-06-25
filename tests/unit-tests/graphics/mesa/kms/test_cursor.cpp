/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/platforms/mesa/server/kms/cursor.h"
#include "src/platforms/mesa/server/kms/kms_output.h"
#include "src/platforms/mesa/server/kms/kms_output_container.h"
#include "src/platforms/mesa/server/kms/kms_display_configuration.h"

#include "mir/graphics/cursor_image.h"

#include "mir/test/doubles/mock_gbm.h"
#include "mir/test/fake_shared.h"
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

namespace
{

struct StubKMSOutputContainer : public mgm::KMSOutputContainer
{
    StubKMSOutputContainer()
        : outputs{
            {10, std::make_shared<testing::NiceMock<MockKMSOutput>>()},
            {11, std::make_shared<testing::NiceMock<MockKMSOutput>>()},
            {12, std::make_shared<testing::NiceMock<MockKMSOutput>>()}}
    {
    }

    std::shared_ptr<mgm::KMSOutput> get_kms_output_for(uint32_t connector_id)
    {
        return outputs[connector_id];
    }

    void for_each_output(std::function<void(mgm::KMSOutput&)> functor) const
    {
        for (auto const& pair : outputs)
            functor(*pair.second);
    }

    void verify_and_clear_expectations()
    {
        for (auto const& pair : outputs)
            ::testing::Mock::VerifyAndClearExpectations(pair.second.get());
    }

    std::unordered_map<uint32_t,std::shared_ptr<testing::NiceMock<MockKMSOutput>>> outputs;
};

struct StubKMSDisplayConfiguration : public mgm::KMSDisplayConfiguration
{
    StubKMSDisplayConfiguration()
        : card_id{1}
    {
        outputs.push_back(
            {
                mg::DisplayConfigurationOutputId{10},
                card_id,
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
                mir_orientation_normal
            });
        outputs.push_back(
            {
                mg::DisplayConfigurationOutputId{11},
                card_id,
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
                mir_orientation_normal
            });
        outputs.push_back(
            {
                mg::DisplayConfigurationOutputId{12},
                card_id,
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
                mir_orientation_right
            });
    }

    void for_each_card(std::function<void(mg::DisplayConfigurationCard const&)> f) const override
    {
        f({card_id, outputs.size()});
    }

    void for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const override
    {
        for (auto const& output : outputs)
            f(output);
    }

    void for_each_output(std::function<void(mg::UserDisplayConfigurationOutput&)> f) override
    {
        for (auto& output : outputs)
        {
            mg::UserDisplayConfigurationOutput user(output);
            f(user);
        }
    }

    uint32_t get_kms_connector_id(mg::DisplayConfigurationOutputId id) const override
    {
        return id.as_value();
    }

    size_t get_kms_mode_index(mg::DisplayConfigurationOutputId, size_t conf_mode_index) const override
    {
        return conf_mode_index;
    }

    void update()
    {
    }

    void set_orentation_of_output(mg::DisplayConfigurationOutputId id, MirOrientation orientation)
    {
        auto output = std::find_if(outputs.begin(), outputs.end(),
                                   [id] (mg::DisplayConfigurationOutput const& output) -> bool {return output.id == id;});
        if (output != outputs.end())
        {
            output->orientation = orientation;
        }
    }

    mg::DisplayConfigurationCardId card_id;
    std::vector<mg::DisplayConfigurationOutput> outputs;
};

struct StubCurrentConfiguration : public mgm::CurrentConfiguration
{
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
void const* StubCursorImage::image_data = reinterpret_cast<void*>(&StubCursorImage::image_data);

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

    MesaCursorTest()
        : cursor{mock_gbm.fake_gbm.device, output_container,
            mt::fake_shared(current_configuration),
            mt::fake_shared(stub_image)}
    {
    }

    StubCurrentConfiguration current_configuration;
    StubCursorImage stub_image;
    StubKMSOutputContainer output_container;
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
    size_t const cursor_side{64};
    EXPECT_CALL(mock_gbm, gbm_bo_create(mock_gbm.fake_gbm.device,
                                        cursor_side, cursor_side,
                                        GBM_FORMAT_ARGB8888,
                                        GBM_BO_USE_CURSOR | GBM_BO_USE_WRITE));

    mgm::Cursor cursor_tmp{mock_gbm.fake_gbm.device, output_container,
        std::make_shared<StubCurrentConfiguration>(),
        std::make_shared<StubCursorImage>()};
}

TEST_F(MesaCursorTest, queries_received_cursor_size)
{
    using namespace ::testing;

    EXPECT_CALL(mock_gbm, gbm_bo_get_width(_));
    EXPECT_CALL(mock_gbm, gbm_bo_get_height(_));

    mgm::Cursor cursor_tmp{mock_gbm.fake_gbm.device, output_container,
        std::make_shared<StubCurrentConfiguration>(),
        std::make_shared<StubCursorImage>()};
}

TEST_F(MesaCursorTest, show_cursor_writes_to_bo)
{
    using namespace testing;

    StubCursorImage image;
    size_t const cursor_side{64};
    geom::Size const cursor_size{cursor_side, cursor_side};
    size_t const cursor_size_bytes{cursor_side * cursor_side * sizeof(uint32_t)};

    EXPECT_CALL(mock_gbm, gbm_bo_write(mock_gbm.fake_gbm.bo, StubCursorImage::image_data, cursor_size_bytes));

    cursor.show(image);
}

// When we upload our 1x1 cursor we should upload a single white pixel and then transparency filling a 64x64 buffer.
MATCHER_P(ContainsASingleWhitePixel, buffersize, "")
{
    auto pixels = static_cast<uint32_t const*>(arg);
    if (pixels[0] != 0xffffffff)
        return false;
    for (decltype(buffersize) i = 1; i < buffersize; i++)
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

    mgm::Cursor cursor_tmp{mock_gbm.fake_gbm.device, output_container,
        std::make_shared<StubCurrentConfiguration>(),
        std::make_shared<SinglePixelCursorImage>()};
}

TEST_F(MesaCursorTest, throws_when_images_are_too_large)
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

    EXPECT_THROW({
        cursor.show(LargeCursorImage());
    }, std::logic_error);
}

TEST_F(MesaCursorTest, forces_cursor_state_on_construction)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[10], move_cursor(geom::Point{0,0}));
    EXPECT_CALL(*output_container.outputs[10], set_cursor(_));
    EXPECT_CALL(*output_container.outputs[11], clear_cursor());
    EXPECT_CALL(*output_container.outputs[12], clear_cursor());

    /* No checking of existing cursor state */
    EXPECT_CALL(*output_container.outputs[10], has_cursor()).Times(0);
    EXPECT_CALL(*output_container.outputs[11], has_cursor()).Times(0);
    EXPECT_CALL(*output_container.outputs[12], has_cursor()).Times(0);

    mgm::Cursor cursor_tmp{mock_gbm.fake_gbm.device, output_container,
       std::make_shared<StubCurrentConfiguration>(),
       std::make_shared<StubCursorImage>()};

    output_container.verify_and_clear_expectations();
}


TEST_F(MesaCursorTest, move_to_sets_clears_cursor_if_needed)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[10], has_cursor())
        .WillOnce(Return(false));
    EXPECT_CALL(*output_container.outputs[10], set_cursor(_));

    EXPECT_CALL(*output_container.outputs[11], has_cursor())
        .WillOnce(Return(true));
    EXPECT_CALL(*output_container.outputs[11], clear_cursor());

    cursor.move_to({10, 10});

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, move_to_doesnt_set_clear_cursor_if_not_needed)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[10], has_cursor())
        .WillOnce(Return(true));
    EXPECT_CALL(*output_container.outputs[10], set_cursor(_))
        .Times(0);

    EXPECT_CALL(*output_container.outputs[11], has_cursor())
        .WillOnce(Return(false));
    EXPECT_CALL(*output_container.outputs[11], clear_cursor())
        .Times(0);

    cursor.move_to({10, 10});

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, move_to_moves_cursor_to_right_output)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[10], move_cursor(geom::Point{10,10}));
    EXPECT_CALL(*output_container.outputs[11], move_cursor(_))
        .Times(0);

    cursor.move_to({10, 10});

    output_container.verify_and_clear_expectations();

    EXPECT_CALL(*output_container.outputs[10], move_cursor(_))
        .Times(0);
    EXPECT_CALL(*output_container.outputs[11], move_cursor(geom::Point{50,100}));

    cursor.move_to({150, 150});

    output_container.verify_and_clear_expectations();

    EXPECT_CALL(*output_container.outputs[10], move_cursor(geom::Point{150,75}));
    EXPECT_CALL(*output_container.outputs[11], move_cursor(geom::Point{50,25}));

    cursor.move_to({150, 75});

    output_container.verify_and_clear_expectations();

    EXPECT_CALL(*output_container.outputs[10], move_cursor(_))
        .Times(0);
    EXPECT_CALL(*output_container.outputs[11], move_cursor(_))
        .Times(0);

    cursor.move_to({-1, -1});

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, moves_properly_to_and_inside_left_rotated_output)
{
    using namespace testing;

    current_configuration.conf.set_orentation_of_output(mg::DisplayConfigurationOutputId{12}, mir_orientation_left);

    EXPECT_CALL(*output_container.outputs[12], move_cursor(geom::Point{112,100}));
    EXPECT_CALL(*output_container.outputs[12], move_cursor(geom::Point{150,96}));

    cursor.move_to({766, 112});
    cursor.move_to({770, 150});

    output_container.verify_and_clear_expectations();
}


TEST_F(MesaCursorTest, moves_properly_to_and_inside_right_rotated_output)
{
    using namespace testing;

    current_configuration.conf.set_orentation_of_output(mg::DisplayConfigurationOutputId{12}, mir_orientation_right);


    EXPECT_CALL(*output_container.outputs[12], move_cursor(geom::Point{688,100}));
    EXPECT_CALL(*output_container.outputs[12], move_cursor(geom::Point{650,104}));

    cursor.move_to({766, 112});
    cursor.move_to({770, 150});

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, moves_properly_to_and_inside_inverted_output)
{
    using namespace testing;

    current_configuration.conf.set_orentation_of_output(mg::DisplayConfigurationOutputId{12}, mir_orientation_inverted);

    EXPECT_CALL(*output_container.outputs[12], move_cursor(geom::Point{700,88}));
    EXPECT_CALL(*output_container.outputs[12], move_cursor(geom::Point{696,50}));

    cursor.move_to({766, 112});
    cursor.move_to({770, 150});

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, hides_cursor_in_all_outputs)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[10], clear_cursor());
    EXPECT_CALL(*output_container.outputs[11], clear_cursor());
    EXPECT_CALL(*output_container.outputs[12], clear_cursor());

    cursor.hide();

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, hidden_cursor_is_not_shown_on_display_when_moved)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[10], clear_cursor());
    EXPECT_CALL(*output_container.outputs[11], clear_cursor());
    EXPECT_CALL(*output_container.outputs[12], clear_cursor());
    EXPECT_CALL(*output_container.outputs[10], move_cursor(_)).Times(0);
    EXPECT_CALL(*output_container.outputs[11], move_cursor(_)).Times(0);
    EXPECT_CALL(*output_container.outputs[12], move_cursor(_)).Times(0);

    cursor.hide();
    cursor.move_to({17, 29});
    
    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, clears_cursor_on_exit)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[10], clear_cursor());
    EXPECT_CALL(*output_container.outputs[11], clear_cursor());
    EXPECT_CALL(*output_container.outputs[12], clear_cursor());
}

TEST_F(MesaCursorTest, cursor_is_shown_at_correct_location_after_suspend_resume)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[10], move_cursor(geom::Point{150,75}));
    EXPECT_CALL(*output_container.outputs[11], move_cursor(geom::Point{50,25}));
    EXPECT_CALL(*output_container.outputs[10], clear_cursor());
    EXPECT_CALL(*output_container.outputs[11], clear_cursor());
    EXPECT_CALL(*output_container.outputs[12], clear_cursor());

    cursor.move_to({150, 75});
    cursor.suspend();

    output_container.verify_and_clear_expectations();

    EXPECT_CALL(*output_container.outputs[10], set_cursor(_));
    EXPECT_CALL(*output_container.outputs[11], set_cursor(_));
    EXPECT_CALL(*output_container.outputs[10], move_cursor(geom::Point{150,75}));
    EXPECT_CALL(*output_container.outputs[11], move_cursor(geom::Point{50,25}));

    cursor.resume();
    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, hidden_cursor_is_not_shown_after_suspend_resume)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[10], clear_cursor()).Times(AnyNumber());
    EXPECT_CALL(*output_container.outputs[11], clear_cursor()).Times(AnyNumber());
    EXPECT_CALL(*output_container.outputs[12], clear_cursor()).Times(AnyNumber());

    cursor.hide();
    cursor.suspend();

    output_container.verify_and_clear_expectations();

    EXPECT_CALL(*output_container.outputs[10], set_cursor(_)).Times(0);
    EXPECT_CALL(*output_container.outputs[11], set_cursor(_)).Times(0);
    EXPECT_CALL(*output_container.outputs[12], set_cursor(_)).Times(0);

    cursor.resume();
    output_container.verify_and_clear_expectations();
}


TEST_F(MesaCursorTest, show_without_param_places_cursor_on_output_output)
{
    using namespace testing;
    EXPECT_CALL(*output_container.outputs[10], clear_cursor());

    cursor.hide();
    output_container.verify_and_clear_expectations();

    EXPECT_CALL(*output_container.outputs[10], set_cursor(_));
    cursor.show();
}

TEST_F(MesaCursorTest, show_cursor_sets_cursor_with_hotspot)
{
    using namespace testing;

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
    EXPECT_CALL(*output_container.outputs[10], set_cursor(_)).Times(AnyNumber());

    // When we set the image with the hotspot, first we should see the cursor move from its initial
    // location, to account for the displacement. Further movement should be offset by the hotspot.
    {
        InSequence seq;
        EXPECT_CALL(*output_container.outputs[10], move_cursor(initial_buffer_location));
        EXPECT_CALL(*output_container.outputs[10], move_cursor(expected_buffer_location_1));
        EXPECT_CALL(*output_container.outputs[10], move_cursor(expected_buffer_location_2));
    }

    cursor.show(HotspotCursor());
    cursor.move_to(cursor_location_1);
    cursor.move_to(cursor_location_2);
}

