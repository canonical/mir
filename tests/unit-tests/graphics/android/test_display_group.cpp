/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/platforms/android/server/display_buffer.h"
#include "src/platforms/android/server/display_group.h"
#include "src/platforms/android/server/gl_context.h"
#include "src/platforms/android/server/android_format_conversion-inl.h"
#include "mir_test_doubles/mock_display_device.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/null_display_buffer.h"
#include "mir_test_doubles/stub_renderable_list_compositor.h"
#include "mir_test_doubles/stub_swapping_gl_context.h"
#include "mir_test_doubles/stub_renderable.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_gl.h"
#include "mir/graphics/android/mir_native_window.h"
#include "mir_test_doubles/stub_driver_interpreter.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/stub_gl_config.h"
#include "mir_test_doubles/mock_framebuffer_bundle.h"
#include "mir_test_doubles/stub_gl_program_factory.h"
#include "mir_test_doubles/mock_hwc_device_wrapper.h"
#include "mir_test/fake_shared.h"
#include <memory>

namespace geom=mir::geometry;
namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace mt=mir::test;

namespace
{
struct StubConfigurableDB : mga::ConfigurableDisplayBuffer
{
    StubConfigurableDB(mga::DisplayName name) :
        name(name)
    {
    }

    geom::Rectangle view_area() const override { return geom::Rectangle(); }
    void make_current() override {}
    void release_current() override {}
    void gl_swap_buffers() override {}
    bool post_renderables_if_optimizable(mg::RenderableList const&) override { return false; }
    MirOrientation orientation() const override { return mir_orientation_normal; }
    bool uses_alpha() const override { return false; }
    void configure(MirPowerMode, MirOrientation) override {}
    mga::DisplayContents contents() const override
    {
        return mga::DisplayContents{name, list, context, compositor};
    }
    MirPowerMode power_mode() const override { return mir_power_mode_on; }

    mga::DisplayName mutable name;
    mga::LayerList  mutable list{std::make_shared<mga::IntegerSourceCrop>(), {}};
    mtd::StubRenderableListCompositor mutable compositor;
    mtd::StubSwappingGLContext mutable context;
};

struct DisplayGroup : public ::testing::Test
{
    mga::DisplayName primary_name{mga::DisplayName::primary};
    mga::DisplayName external_name{mga::DisplayName::external};
    std::unique_ptr<StubConfigurableDB> primary{new StubConfigurableDB(primary_name)};
    testing::NiceMock<mtd::MockDisplayDevice> mock_device;
};
}

TEST_F(DisplayGroup, db_additions_and_removals)
{
    mga::DisplayGroup group(mt::fake_shared(mock_device), std::move(primary));
 
    //hwc api does not allow removing primary
    EXPECT_THROW({
        group.remove(mga::DisplayName::primary);
    }, std::logic_error);

    group.add(external_name, std::unique_ptr<StubConfigurableDB>(new StubConfigurableDB(external_name)));

    //can replace primary or external
    group.add(primary_name, std::unique_ptr<StubConfigurableDB>(new StubConfigurableDB(primary_name)));
    group.add(external_name, std::unique_ptr<StubConfigurableDB>(new StubConfigurableDB(external_name)));
    group.remove(external_name);
    group.remove(external_name); //no external display, should be ignored
}

TEST_F(DisplayGroup, posts_content_of_dbs)
{
    using namespace testing;
    InSequence seq;
    EXPECT_CALL(mock_device, commit(SizeIs(1)));
    EXPECT_CALL(mock_device, commit(SizeIs(2)));
    EXPECT_CALL(mock_device, commit(SizeIs(1)));

    mga::DisplayGroup group(mt::fake_shared(mock_device), std::move(primary));
    group.post();

    group.add(external_name, std::unique_ptr<StubConfigurableDB>(new StubConfigurableDB(external_name)));
    group.post();

    group.remove(mga::DisplayName::external);
    group.post();
}


#if 0
TEST_F(DisplayGroup, can_post_update_with_gl_only)
{
    using namespace testing;
    mga::DisplayName external{mga::DisplayName::external};
    std::unique_ptr<mga::LayerList> list(new mga::LayerList(std::make_shared<mga::IntegerSourceCrop>(), {}));
    EXPECT_CALL(*mock_display_device, commit(external, Ref(*list), _, _));

    mga::DisplayGroup db{
        external,
        std::move(list),
        mock_fb_bundle,
        mock_display_device,
        native_window,
        *gl_context,
        stub_program_factory,
        orientation,
        mga::OverlayOptimization::enabled};

    db.gl_swap_buffers();
}

TEST_F(DisplayGroup, rejects_commit_if_list_doesnt_need_commit)
{
    using namespace testing;
    auto buffer1 = std::make_shared<mtd::StubRenderable>();
    auto buffer2 = std::make_shared<mtd::StubRenderable>();
    auto buffer3 = std::make_shared<mtd::StubRenderable>();

    ON_CALL(*mock_display_device, compatible_renderlist(_))
        .WillByDefault(Return(true));
    ON_CALL(*mock_display_device, commit(_,_,_,_))
        .WillByDefault(Invoke([](
            mga::DisplayName,
            mga::LayerList& list,
            mga::SwappingGLContext const&,
            mga::RenderableListCompositor const&)
        {
            auto native_list = list.native_list();
            for (auto i = 0u; i < native_list->numHwLayers; i++)
            {
                if (native_list->hwLayers[i].compositionType == HWC_FRAMEBUFFER)
                    native_list->hwLayers[i].compositionType = HWC_OVERLAY;
            }
        }));

    mg::RenderableList renderlist{buffer1, buffer2};
    EXPECT_TRUE(db.post_renderables_if_optimizable(renderlist)); 
    EXPECT_FALSE(db.post_renderables_if_optimizable(renderlist)); 

    renderlist = mg::RenderableList{buffer2, buffer1}; //ordering changed
    EXPECT_TRUE(db.post_renderables_if_optimizable(renderlist)); 
    EXPECT_FALSE(db.post_renderables_if_optimizable(renderlist)); 

    renderlist = mg::RenderableList{buffer3, buffer1}; //buffer changed
    EXPECT_TRUE(db.post_renderables_if_optimizable(renderlist)); 
    EXPECT_FALSE(db.post_renderables_if_optimizable(renderlist)); 
}
#endif
