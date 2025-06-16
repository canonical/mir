/*
 * Copyright © Canonical Ltd.
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
 */

#include "src/platforms/atomic-kms/server/kms/cursor.h"
#include "src/platforms/atomic-kms/server/kms/kms_output.h"
#include "src/platforms/atomic-kms/server/kms/kms_output_container.h"
#include "src/platforms/atomic-kms/server/kms/kms_display_configuration.h"
#include "mir/graphics/cursor_image.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/doubles/mock_gbm.h"
#include "mir/test/doubles/mock_drm.h"
#include "mir/test/fake_shared.h"
#include "mock_kms_output.h"
#include "kms/atomic_kms_output.h"

#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mga = mir::graphics::atomic;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;
using mt::MockKMSOutput;
using namespace ::testing;

namespace
{

class StubKMSOutputContainer : public mga::KMSOutputContainer
{
public:
    void add_output(geom::Size size)
    {
        auto const output = std::make_shared<testing::NiceMock<MockKMSOutput>>();
        outputs.push_back(output);

        auto& out = *output;
        ON_CALL(out, size())
            .WillByDefault(Return(size));
        ON_CALL(out, has_cursor_image())
            .WillByDefault(Return(true));
        ON_CALL(out, clear_cursor())
            .WillByDefault(Return(true));
    }

    void for_each_output(std::function<void(std::shared_ptr<mga::KMSOutput> const&)> functor) const override
    {
        for (auto const& output : outputs)
            functor(output);
    }

    void update_from_hardware_state() override
    {
    }

    std::vector<std::shared_ptr<testing::NiceMock<MockKMSOutput>>> outputs;
};

class StubKMSDisplayConfiguration : public mga::KMSDisplayConfiguration
{
public:
    explicit StubKMSDisplayConfiguration(StubKMSOutputContainer& container)
        : mga::KMSDisplayConfiguration(),
          container{container},
          stub_config{
            {{
                mg::DisplayConfigurationOutputId{0},
                mg::DisplayConfigurationCardId{},
                mg::DisplayConfigurationLogicalGroupId{},
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
                mg::DisplayConfigurationLogicalGroupId{},
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
                mg::DisplayConfigurationLogicalGroupId{},
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
        stub_config.for_each_output([&container](auto const& conf)
            {
                container.add_output(conf.modes[conf.current_mode_index].size);
            });
        container.for_each_output(
            [this](auto const& output)
            {
                outputs.push_back(output);
            });
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

    std::shared_ptr<mga::KMSOutput> get_output_for(mg::DisplayConfigurationOutputId id) const override
    {
        return outputs[id.as_value()];
    }

    size_t get_kms_mode_index(mg::DisplayConfigurationOutputId, size_t conf_mode_index) const override
    {
        return conf_mode_index;
    }

    void update() override
    {
    }

    StubKMSOutputContainer& container;
    mtd::StubDisplayConfig stub_config;
    std::vector<std::shared_ptr<mga::KMSOutput>> outputs;
};

class StubCurrentConfiguration : public mga::CurrentConfiguration
{
public:
    explicit StubCurrentConfiguration(StubKMSOutputContainer& container)
        : conf(container)
    {
    }

    void with_current_configuration_do(
        std::function<void(mga::KMSDisplayConfiguration const&)> const& exec)
    {
        exec(conf);
    }

    StubKMSDisplayConfiguration conf;
};

class StubCursorImage : public mg::CursorImage
{
public:
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

class AtomicKmsCursorTest : public Test
{
public:
    AtomicKmsCursorTest()
        : cursor{output_container,
          mt::fake_shared(current_configuration)}
    {
        ON_CALL(mock_drm, drmGetCap(_, DRM_CAP_CURSOR_WIDTH, _))
            .WillByDefault(Invoke([](int , uint64_t , uint64_t *value)
              {
                  *value = 64;
                  return 0;
              }));
        ON_CALL(mock_drm, drmGetCap(_, DRM_CAP_CURSOR_HEIGHT, _))
            .WillByDefault(Invoke([](int , uint64_t , uint64_t *value)
              {
                  *value = 64;
                  return 0;
              }));
    }

    struct MockGBM : NiceMock<mtd::MockGBM>
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
    NiceMock<mtd::MockDRM> mock_drm;
    StubKMSOutputContainer output_container;
    StubCurrentConfiguration current_configuration{output_container};
    mga::Cursor cursor;

};
}

TEST_F(AtomicKmsCursorTest, if_shown_cursor_renderable_is_not_null)
{
    using namespace testing;

    auto image = std::make_shared<StubCursorImage>();
    cursor.show(image);

    EXPECT_THAT(cursor.renderable(), NotNull());
}

TEST_F(AtomicKmsCursorTest, cursor_renderable_buffer_is_not_null)
{
    using namespace testing;

    auto image = std::make_shared<StubCursorImage>();
    cursor.show(image);

    EXPECT_THAT(cursor.renderable()->buffer(), NotNull());
}

TEST_F(AtomicKmsCursorTest, cursor_renderable_id_is_not_null)
{
    using namespace testing;

    auto image = std::make_shared<StubCursorImage>();
    cursor.show(image);

    EXPECT_THAT(cursor.renderable()->id(), NotNull());
}

TEST_F(AtomicKmsCursorTest, cursor_renderable_screen_position_is_expected)
{
    using namespace testing;

    auto image = std::make_shared<StubCursorImage>();
    cursor.move_to(geom::Point{50, 50});
    cursor.show(image);

    EXPECT_THAT(cursor.renderable()->screen_position(), Eq(
        geom::Rectangle({50, 50}, {image->size()})));
}

TEST_F(AtomicKmsCursorTest, cursor_renderable_src_bounds_is_expected)
{
    using namespace testing;

    auto image = std::make_shared<StubCursorImage>();
    cursor.move_to(geom::Point{50, 50});
    cursor.show(image);

    EXPECT_THAT(cursor.renderable()->src_bounds(), Eq(
        geom::RectangleD({0, 0}, image->size())));
}

TEST_F(AtomicKmsCursorTest, cursor_renderable_clip_area_is_unset)
{
    using namespace testing;

    auto image = std::make_shared<StubCursorImage>();
    cursor.show(image);

    EXPECT_THAT(cursor.renderable()->clip_area(), Eq(std::nullopt));
}

TEST_F(AtomicKmsCursorTest, cursor_renderable_alpha_is_one)
{
    using namespace testing;

    auto image = std::make_shared<StubCursorImage>();
    cursor.show(image);

    EXPECT_THAT(cursor.renderable()->alpha(), Eq(1));
}

TEST_F(AtomicKmsCursorTest, cursor_renderable_transformation_is_identity)
{
    using namespace testing;

    auto image = std::make_shared<StubCursorImage>();
    cursor.show(image);

    EXPECT_THAT(cursor.renderable()->transformation(), Eq(glm::mat4(1.f)));
}

TEST_F(AtomicKmsCursorTest, cursor_renderable_shaped_is_true)
{
    using namespace testing;

    auto image = std::make_shared<StubCursorImage>();
    cursor.show(image);

    EXPECT_THAT(cursor.renderable()->shaped(), Eq(true));
}

TEST_F(AtomicKmsCursorTest, cursor_renderable_surface_is_nullopt)
{
    using namespace testing;

    auto image = std::make_shared<StubCursorImage>();
    cursor.show(image);

    EXPECT_THAT(cursor.renderable()->surface_if_any(), Eq(std::nullopt));
}

TEST_F(AtomicKmsCursorTest, if_hidden_cursor_renderable_is_null)
{
    using namespace testing;

    auto image = std::make_shared<StubCursorImage>();
    cursor.show(image);
    cursor.hide();

    EXPECT_THAT(cursor.renderable(), IsNull());
}

TEST_F(AtomicKmsCursorTest, does_not_need_compositing)
{
    EXPECT_THAT(cursor.needs_compositing(), Eq(false));
}


TEST_F(AtomicKmsCursorTest, renderable_has_mirror_mode_none)
{
    using namespace testing;

    auto image = std::make_shared<StubCursorImage>();
    cursor.show(image);

    EXPECT_THAT(cursor.renderable()->mirror_mode(), Eq(mir_mirror_mode_none));
}