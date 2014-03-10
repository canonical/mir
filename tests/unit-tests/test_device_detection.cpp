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

#include "mir/device_detector.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
struct MockOps : mir::PropertiesWrapper
{
    MOCK_CONST_METHOD3(property_get, int(
        char const[PROP_NAME_MAX], char[PROP_VALUE_MAX], char const[PROP_VALUE_MAX]));
};
}

TEST(DeviceDetection, detects_device)
{
    using namespace testing;
    static char const default_str[] = "";
    static char const name_str[] = "tunafish";

    MockOps mock_ops;
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device"), _, StrEq(default_str)))
        .Times(1)
        .WillOnce(Invoke([&]
        (char const[PROP_NAME_MAX], char value[PROP_VALUE_MAX], char const[PROP_VALUE_MAX])
        {
            memcpy(value, name_str, sizeof(name_str));
            return 0;
        }));

    mir::DeviceDetector detector(mock_ops);
    EXPECT_TRUE(detector.android_device_present());
    EXPECT_EQ(std::string{name_str}, detector.device_name());
}

TEST(DeviceDetection, does_not_detect_device)
{
    using namespace testing;
    static char const default_str[] = "";

    MockOps mock_ops;
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device"), _, StrEq(default_str)))
        .Times(1)
        .WillOnce(Invoke([&]
        (char const[PROP_NAME_MAX], char value[PROP_VALUE_MAX], char const default_value[PROP_VALUE_MAX])
        {
            memcpy(value, default_value, sizeof(*default_value));
            return 0;
        }));

    mir::DeviceDetector detector(mock_ops);
    EXPECT_FALSE(detector.android_device_present());
    EXPECT_EQ(std::string{}, detector.device_name());
}
