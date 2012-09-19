/*
 * Copyright © 2012 Canonical Ltd.
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

#include "mir/graphics/display.h"
#include "mir/graphics/platform.h"

#include <gtest/gtest.h>
#include <memory>

namespace mg=mir::graphics;

TEST(AndroidFramebufferIntegration, init_does_not_throw)
{
    using namespace testing;

    EXPECT_NO_THROW(
    {
        auto platform = mg::create_platform();
        auto display = mg::create_display(platform);
        display->post_update();
    });

}
