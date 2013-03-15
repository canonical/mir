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
 *
 */

#include "src/server/graphics/android/display_selector.h"
#include "src/server/graphics/android/android_platform.h"
#include <gmock/gmock.h>

namespace mga=mir::graphics::android;
namespace mg=mir::graphics;

namespace
{
struct MockDisplaySelector : public mga::DisplaySelector
{
    MOCK_METHOD0(primary_display, std::shared_ptr<mg::Display>());
};
}

TEST(AndroidGraphicsPlatform, test_display_creation)
{
    using namespace testing;
    auto selector = std::make_shared<MockDisplaySelector>(); 
    auto platform = std::make_shared<mga::AndroidPlatform>(selector);

    EXPECT_CALL(*selector, primary_display())
        .Times(1)
        .WillOnce(Return(std::shared_ptr<mg::Display>()));
    platform->create_display();
}
