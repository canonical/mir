/*
 * Copyright © 2014 Canonical Ltd.
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

#include "src/platform/graphics/android/device_detector.h"

namespace
{
struct MockOps : mga::AndroidPropertiesOps
{
    MOCK_CONST_METHOD3(property_get, int(char const*, char*, char const*));
};
}

TEST(DeviceDetection, detects_device)
{
    static char const default_str[] = "";
    static char const name_str[] = "tunafish";

    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device", _, StrEq(default_str))
        .Times(1)
        .WillOnce(SetArg<2>(default_str));

    DeviceDetector detector(mock_ops);
    EXPECT_TRUE(detector.android_device_present());
    EXPECT_STREQ(detector.device_name(), "tunafish");
}

TEST(DeviceDetection, detects_no_android_device)
{
    static char const default_str[] = "";
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device", _, StrEq(default_str))
        .Times(1)
        .WillOnce(SetArg<2>(default_str));

    EXPECT_FALSE(detector.android_device_present());
    EXPECT_STREQ(detector.device_name(), std::string{});
}
