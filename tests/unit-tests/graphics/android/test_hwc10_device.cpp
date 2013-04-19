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

#include "src/server/graphics/android/hwc10_device.h"
#include "mir_test_doubles/mock_display_support_provider.h"
#include "mir_test_doubles/mock_hwc_composer_device_1.h"
#include "mir_test_doubles/mock_hwc_organizer.h"
#include "mir_test_doubles/mock_android_buffer.h"
#include <gtest/gtest.h>

namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

class HWC10Device : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;
        test_size = geom::Size{geom::Width{88}, geom::Height{4}};
        mock_device = std::make_shared<testing::NiceMock<mtd::MockHWCComposerDevice1>>();
        mock_organizer = std::make_shared<mtd::MockHWCOrganizer>();
        mock_fbdev = std::make_shared<mtd::MockDisplaySupportProvider>();
        ON_CALL(*mock_fbdev, display_size())
            .WillByDefault(Return(test_size)); 
    }

    geom::Size test_size;
    std::shared_ptr<mtd::MockHWCOrganizer> mock_organizer;
    std::shared_ptr<mtd::MockHWCComposerDevice1> mock_device;
    std::shared_ptr<mtd::MockDisplaySupportProvider> mock_fbdev;
};

TEST_F(HWC10Device, hwc10_gets_size_from_fb_dev)
{
    mga::HWC10Device device(mock_device, mock_organizer, mock_fbdev);

    auto size = device.display_size();
    EXPECT_EQ(test_size, size);
}
