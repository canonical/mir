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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/platform/graphics/android/hwc_layerlist.h"
#include "src/platform/graphics/android/hwc_layers.h"
#include "mir_test_doubles/mock_buffer.h"
#include "hwc_struct_helper-inl.h"
#include "mir_test_doubles/mock_android_native_buffer.h"
#include "mir_test/fake_shared.h"
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

namespace mir
{
namespace test
{
namespace doubles
{
struct MockRenderable : public mg::Renderable
{
    MOCK_CONST_METHOD0(buffer, std::shared_ptr<mg::Buffer>());
    MOCK_CONST_METHOD0(alpha_enabled, bool());
    MOCK_CONST_METHOD0(screen_position, geom::Rectangle());
};
}
}
}

class HWCLayerListTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;

        native_handle_1 = std::make_shared<mtd::StubAndroidNativeBuffer>();
        native_handle_1->anwb()->width = width;
        native_handle_1->anwb()->height = height;

        ON_CALL(mock_buffer, size())
            .WillByDefault(Return(buffer_size));
        ON_CALL(mock_buffer, native_buffer_handle())
            .WillByDefault(Return(native_handle_1));
        ON_CALL(mock_renderable, buffer())
            .WillByDefault(Return(mt::fake_shared(mock_buffer)));
        ON_CALL(mock_renderable, alpha_enabled())
            .WillByDefault(Return(alpha_enabled));
        ON_CALL(mock_renderable, screen_position())
            .WillByDefault(Return(screen_position));
    }

    geom::Size buffer_size{333, 444};
    geom::Rectangle screen_position{{9,8},{245, 250}};
    bool alpha_enabled{true};
    int width{432};
    int height{876};
    std::shared_ptr<mg::NativeBuffer> native_handle_1;
    testing::NiceMock<mtd::MockRenderable> mock_renderable;
    testing::NiceMock<mtd::MockBuffer> mock_buffer;
};


/* Tests without HWC_FRAMEBUFFER_TARGET */
/* an empty list should have a skipped layer. This will force a GL render on hwc set */
TEST_F(HWCLayerListTest, hwc10_list_defaults)
{
    mga::LayerList layerlist({});

    auto list = layerlist.native_list();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    ASSERT_EQ(1, list->numHwLayers);

    mga::ForceGLLayer surface_layer;
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
}

TEST_F(HWCLayerListTest, hwc10_list_initializer)
{
    using namespace testing;
    mga::ForceGLLayer surface_layer;
    mga::LayerList layerlist({surface_layer, surface_layer});

    auto list = layerlist.native_list();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    ASSERT_EQ(2, list->numHwLayers);

    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, hwc10_list_update)
{
    using namespace testing;
    mga::CompositionLayer surface_layer{mock_renderable}; 
    mga::ForceGLLayer skip_layer;
    mga::LayerList layerlist({});

    std::list<std::shared_ptr<mg::Renderable>> updated_list({
        mt::fake_shared(mock_renderable),
        mt::fake_shared(mock_renderable)
    });

    layerlist.update_composition_layers(std::move(updated_list));

    auto list = layerlist.native_list();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    ASSERT_EQ(2, list->numHwLayers);
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[1]));

    /* likewise, if we update to an empty list, we revert to the default */
    std::list<std::shared_ptr<mg::Renderable>> empty_update{};
    layerlist.update_composition_layers(std::move(empty_update));

    list = layerlist.native_list();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    ASSERT_EQ(1, list->numHwLayers);
    EXPECT_THAT(skip_layer, MatchesLayer(list->hwLayers[0]));
}

/* tests with a FRAMEBUFFER_TARGET present */
TEST_F(HWCLayerListTest, fb_target_not_at_end_throws)
{
    using namespace testing;
    //a fb target that is not last is an improperly formed layerlist
    EXPECT_THROW({
        mga::LayerList layerlist({mga::ForceGLLayer{}, mga::FramebufferLayer{}, mga::ForceGLLayer{}});
    }, std::runtime_error);

    //two fb targets is also an improperly formed layerlist
    EXPECT_THROW({
        mga::LayerList layerlist({mga::ForceGLLayer{}, mga::FramebufferLayer{}, mga::FramebufferLayer{}});
    }, std::runtime_error);
}

TEST_F(HWCLayerListTest, fbtarget_list_initialize)
{
    using namespace testing;
    mga::ForceGLLayer surface_layer;
    mga::FramebufferLayer target_layer;

    mga::LayerList layerlist({surface_layer, target_layer});
    auto list = layerlist.native_list();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    ASSERT_EQ(2, list->numHwLayers);
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[1]));

    //add a skipped layer if list is just a target layer
    mga::LayerList no_skip_layerlist({target_layer});
    list = no_skip_layerlist.native_list();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    ASSERT_EQ(2, list->numHwLayers);

    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, fb_target_set)
{
    mga::LayerList layerlist({mga::FramebufferLayer{}});

    layerlist.set_fb_target(native_handle_1);

    auto list = layerlist.native_list();
    ASSERT_EQ(2, list->numHwLayers);
    mga::ForceGLLayer skip_layer;
    mga::FramebufferLayer updated_target_layer(*native_handle_1);
    EXPECT_THAT(skip_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(updated_target_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, fbtarget_list_update)
{
    using namespace testing;
    mga::CompositionLayer surface_layer(mock_renderable);
    mga::FramebufferLayer target_layer;

    mga::LayerList layerlist({target_layer});

    /* set non-default renderlist */
    std::list<std::shared_ptr<mg::Renderable>> updated_list({
        mt::fake_shared(mock_renderable),
        mt::fake_shared(mock_renderable)
    });
    layerlist.update_composition_layers(std::move(updated_list));

    auto list = layerlist.native_list();
    EXPECT_EQ(-1, list->retireFenceFd);
    EXPECT_EQ(HWC_GEOMETRY_CHANGED, list->flags);
    EXPECT_NE(nullptr, list->dpy);
    EXPECT_NE(nullptr, list->sur);
    ASSERT_EQ(3, list->numHwLayers);
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[1]));
    EXPECT_THAT(target_layer, MatchesLayer(list->hwLayers[2]));

    /* update FB target */
    layerlist.set_fb_target(native_handle_1);

    mga::FramebufferLayer handle_target_layer{*native_handle_1};
    list = layerlist.native_list();
    ASSERT_EQ(3, list->numHwLayers);
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(surface_layer, MatchesLayer(list->hwLayers[1]));
    EXPECT_THAT(handle_target_layer, MatchesLayer(list->hwLayers[2]));

    /* reset default */
    std::list<std::shared_ptr<mg::Renderable>> empty_list({});
    layerlist.update_composition_layers(std::move(empty_list));

    list = layerlist.native_list();
    ASSERT_EQ(2, list->numHwLayers);
    mga::ForceGLLayer force_gl_layer;
    EXPECT_THAT(force_gl_layer, MatchesLayer(list->hwLayers[0]));
    EXPECT_THAT(handle_target_layer, MatchesLayer(list->hwLayers[1]));
}

TEST_F(HWCLayerListTest, get_fb_fence)
{
    int release_fence = 381;
    mga::FramebufferLayer target_layer;
    mga::LayerList layerlist({target_layer});

    auto list = layerlist.native_list();
    ASSERT_EQ(2, list->numHwLayers);
    list->hwLayers[1].releaseFenceFd = release_fence;

    EXPECT_EQ(release_fence, layerlist.framebuffer_fence());
}
