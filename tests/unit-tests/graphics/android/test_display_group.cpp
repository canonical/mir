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

#include "src/platforms/android/server/configurable_display_buffer.h"
#include "src/platforms/android/server/display_group.h"
#include "src/platforms/android/server/display_disconnected_exception.h"
#include "mir/test/doubles/mock_display_device.h"
#include "mir/test/doubles/stub_renderable_list_compositor.h"
#include "mir/test/doubles/stub_swapping_gl_context.h"
#include "mir/test/fake_shared.h"
#include <memory>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace mt=mir::test;

namespace
{
struct StubConfigurableDB : mga::ConfigurableDisplayBuffer, mg::NativeDisplayBuffer
{
    mir::geometry::Rectangle view_area() const override { return {}; }
    bool post_renderables_if_optimizable(mg::RenderableList const&) override { return false; }
    MirOrientation orientation() const override { return mir_orientation_normal; }
    mg::NativeDisplayBuffer* native_display_buffer() override { return this; }
    void configure(MirPowerMode, MirOrientation, mir::geometry::Displacement) override {}
    mga::DisplayContents contents() override
    {
        return mga::DisplayContents{mga::DisplayName::primary, list, offset, context, compositor};
    }
    MirPowerMode power_mode() const override { return mir_power_mode_on; }
    mga::LayerList mutable list{std::make_shared<mga::IntegerSourceCrop>(), {}, offset};
    mtd::StubRenderableListCompositor mutable compositor;
    mtd::StubSwappingGLContext mutable context;
    mir::geometry::Displacement offset;
};
}

TEST(DisplayGroup, db_additions_and_removals)
{
    using namespace testing;
    NiceMock<mtd::MockDisplayDevice> mock_device;
    InSequence seq;
    EXPECT_CALL(mock_device, commit(SizeIs(1)));
    EXPECT_CALL(mock_device, commit(SizeIs(2)));
    EXPECT_CALL(mock_device, commit(SizeIs(2)));
    EXPECT_CALL(mock_device, commit(SizeIs(1)));

    mga::DisplayGroup group(mt::fake_shared(mock_device), std::unique_ptr<StubConfigurableDB>{new StubConfigurableDB});
    group.post();
 
    //hwc api does not allow removing primary
    EXPECT_THROW({
        group.remove(mga::DisplayName::primary);
    }, std::logic_error);

    EXPECT_FALSE(group.display_present(mga::DisplayName::external));
    group.add(mga::DisplayName::external, std::unique_ptr<StubConfigurableDB>(new StubConfigurableDB));
    EXPECT_TRUE(group.display_present(mga::DisplayName::external));
    group.post();

    //can replace primary or external
    group.add(mga::DisplayName::primary, std::unique_ptr<StubConfigurableDB>(new StubConfigurableDB));
    group.add(mga::DisplayName::external, std::unique_ptr<StubConfigurableDB>(new StubConfigurableDB));
    group.post();
    group.remove(mga::DisplayName::external);
    group.remove(mga::DisplayName::external);
    group.post();
}

//lp: 1474891, 1498550: If the driver is processing the external display in set,
//and it gets a hotplug event removing the external display, set() will throw, which we should ignore
TEST(DisplayGroup, group_ignores_throws_during_hotplug)
{
    using namespace testing;
    NiceMock<mtd::MockDisplayDevice> mock_device;
    mga::DisplayGroup group(mt::fake_shared(mock_device), std::make_unique<StubConfigurableDB>());
    EXPECT_CALL(mock_device, commit(_))
        .WillOnce(Throw(mga::DisplayDisconnectedException()))
        .WillOnce(Throw(std::runtime_error("")));

    EXPECT_NO_THROW({group.post();});

    EXPECT_THROW({
        group.post();
    }, std::runtime_error);
}
