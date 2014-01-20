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

#include "src/platform/graphics/mesa/cursor.h"
#include "src/platform/graphics/mesa/kms_output.h"
#include "src/platform/graphics/mesa/kms_output_container.h"
#include "src/platform/graphics/mesa/kms_display_configuration.h"

#include "mir_test_doubles/mock_gbm.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <unordered_map>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{

struct MockKMSOutput : public mgm::KMSOutput
{
    MOCK_METHOD0(reset, void());
    MOCK_METHOD2(configure, void(geom::Displacement, size_t));
    MOCK_CONST_METHOD0(size, geom::Size());

    MOCK_METHOD1(set_crtc, bool(uint32_t));
    MOCK_METHOD0(clear_crtc, void());
    MOCK_METHOD1(schedule_page_flip, bool(uint32_t));
    MOCK_METHOD0(wait_for_page_flip, void());

    MOCK_METHOD1(set_cursor, void(gbm_bo*));
    MOCK_METHOD1(move_cursor, void(geom::Point));
    MOCK_METHOD0(clear_cursor, void());
    MOCK_CONST_METHOD0(has_cursor, bool());

    MOCK_METHOD1(set_power_mode, void(MirPowerMode));
};

struct StubKMSOutputContainer : public mgm::KMSOutputContainer
{
    StubKMSOutputContainer()
        : outputs{
            {10, std::make_shared<testing::NiceMock<MockKMSOutput>>()},
            {11, std::make_shared<testing::NiceMock<MockKMSOutput>>()}}
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

    void configure_output(mg::DisplayConfigurationOutputId, bool,
                          geom::Point, size_t, MirPixelFormat, MirPowerMode,
                          MirOrientation) override
    {
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

struct MesaCursorTest : public ::testing::Test
{
    MesaCursorTest()
        : cursor{mock_gbm.fake_gbm.device, output_container,
                 std::make_shared<StubCurrentConfiguration>()}
    {
    }

    testing::NiceMock<mtd::MockGBM> mock_gbm;
    StubKMSOutputContainer output_container;
    mgm::Cursor cursor;
};

}

TEST_F(MesaCursorTest, creates_cursor_bo_image)
{
    size_t const cursor_side{64};
    EXPECT_CALL(mock_gbm, gbm_bo_create(mock_gbm.fake_gbm.device,
                                        cursor_side, cursor_side,
                                        GBM_FORMAT_ARGB8888,
                                        GBM_BO_USE_CURSOR_64X64 | GBM_BO_USE_WRITE));

    mgm::Cursor cursor_tmp{mock_gbm.fake_gbm.device, output_container,
                              std::make_shared<StubCurrentConfiguration>()};
}

TEST_F(MesaCursorTest, set_cursor_writes_to_bo)
{
    using namespace testing;

    void* const image{reinterpret_cast<void*>(0x5678)};
    size_t const cursor_side{64};
    geom::Size const cursor_size{cursor_side, cursor_side};
    size_t const cursor_size_bytes{cursor_side * cursor_side * sizeof(uint32_t)};

    EXPECT_CALL(mock_gbm, gbm_bo_write(mock_gbm.fake_gbm.bo, image, cursor_size_bytes));

    cursor.set_image(image, cursor_size);
}

TEST_F(MesaCursorTest, set_cursor_throws_on_incorrect_size)
{
    using namespace testing;

    void* const image{reinterpret_cast<void*>(0x5678)};
    size_t const cursor_side{48};
    geom::Size const cursor_size{cursor_side, cursor_side};

    EXPECT_THROW(
        cursor.set_image(image, cursor_size);
    , std::logic_error);
}

TEST_F(MesaCursorTest, forces_cursor_state_on_construction)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[10], move_cursor(geom::Point{0,0}));
    EXPECT_CALL(*output_container.outputs[10], set_cursor(_));
    EXPECT_CALL(*output_container.outputs[11], clear_cursor());

    /* No checking of existing cursor state */
    EXPECT_CALL(*output_container.outputs[10], has_cursor()).Times(0);
    EXPECT_CALL(*output_container.outputs[11], has_cursor()).Times(0);

    mgm::Cursor cursor_tmp{mock_gbm.fake_gbm.device, output_container,
                              std::make_shared<StubCurrentConfiguration>()};

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

TEST_F(MesaCursorTest, shows_at_last_known_position)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[10], move_cursor(geom::Point{150,75}));
    EXPECT_CALL(*output_container.outputs[11], move_cursor(geom::Point{50,25}));

    cursor.move_to({150, 75});

    output_container.verify_and_clear_expectations();

    EXPECT_CALL(*output_container.outputs[10], move_cursor(geom::Point{150,75}));
    EXPECT_CALL(*output_container.outputs[11], move_cursor(geom::Point{50,25}));

    cursor.show_at_last_known_position();

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, hides_cursor_in_all_outputs)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[10], clear_cursor());
    EXPECT_CALL(*output_container.outputs[11], clear_cursor());

    cursor.hide();

    output_container.verify_and_clear_expectations();
}

TEST_F(MesaCursorTest, clears_cursor_on_exit)
{
    using namespace testing;

    EXPECT_CALL(*output_container.outputs[10], clear_cursor());
    EXPECT_CALL(*output_container.outputs[11], clear_cursor());
}
