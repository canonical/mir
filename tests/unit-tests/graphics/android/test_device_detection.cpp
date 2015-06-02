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

#include "src/platforms/android/server/device_quirks.h"
#include "mir/options/program_option.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mga = mir::graphics::android;
namespace bpo = boost::program_options;

namespace
{
struct MockOps : mga::PropertiesWrapper
{
    MOCK_CONST_METHOD3(property_get, int(
        char const[PROP_NAME_MAX], char[PROP_VALUE_MAX], char const[PROP_VALUE_MAX]));
};
}

TEST(DeviceDetection, two_buffers_by_default)
{
    using namespace testing;
    char const default_str[] = "";
    char const name_str[] = "anydevice";

    MockOps mock_ops;
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device"), _, StrEq(default_str)))
        .Times(1)
        .WillOnce(Invoke([&]
        (char const*, char* value, char const*)
        {
            strncpy(value, name_str, PROP_VALUE_MAX);
            return 0;
        }));

    mga::DeviceQuirks quirks(mock_ops);
    EXPECT_EQ(2u, quirks.num_framebuffers());
}

TEST(DeviceDetection, three_buffers_reported_for_mx3)
{
    using namespace testing;
    char const default_str[] = "";
    char const name_str[] = "mx3";

    MockOps mock_ops;
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device"), _, StrEq(default_str)))
        .Times(1)
        .WillOnce(Invoke([&]
        (char const*, char* value, char const*)
        {
            strncpy(value, name_str, PROP_VALUE_MAX);
            return 0;
        }));

    mga::DeviceQuirks quirks(mock_ops);
    EXPECT_EQ(3u, quirks.num_framebuffers());
}

//LP: 1371619, 1370555
TEST(DeviceDetection, reports_gralloc_reopenable_after_close_by_default)
{
    using namespace testing;
    char const default_str[] = "";
    char const any_name_str[] = "anydevice";

    MockOps mock_ops;
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device"), _, StrEq(default_str)))
        .Times(1)
        .WillOnce(Invoke([&](char const*, char* value, char const*)
        {
            strncpy(value, any_name_str, PROP_VALUE_MAX);
            return 0;
        }));

    mga::DeviceQuirks quirks(mock_ops);
    EXPECT_TRUE(quirks.gralloc_reopenable_after_close());
}

TEST(DeviceDetection, reports_gralloc_not_reopenable_after_close_on_krillin)
{
    using namespace testing;
    char const default_str[] = "";
    char const krillin_name_str[] = "krillin";

    MockOps mock_ops;
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device"), _, StrEq(default_str)))
        .Times(1)
        .WillOnce(Invoke([&](char const*, char* value, char const*)
        {
            strncpy(value, krillin_name_str, PROP_VALUE_MAX);
            return 0;
        }));

    mga::DeviceQuirks quirks(mock_ops);
    EXPECT_FALSE(quirks.gralloc_reopenable_after_close());
}

TEST(DeviceDetection, aligns_width_on_vegetahd)
{
    using namespace testing;
    char const default_str[] = "";
    char const vegetahd_name_str[] = "vegetahd";

    MockOps mock_ops;
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device"), _, StrEq(default_str)))
        .Times(1)
        .WillOnce(Invoke([&](char const*, char* value, char const*)
        {
            strncpy(value, vegetahd_name_str, PROP_VALUE_MAX);
            return 0;
        }));

    mga::DeviceQuirks quirks(mock_ops);
    EXPECT_THAT(quirks.aligned_width(720), Eq(736));
}

struct DeviceQuirks : testing::Test
{
    void SetUp()
    {
        mga::DeviceQuirks::add_options(desc);
    }

    void disable_num_framebuffers_quirk()
    {
        int const argc = 2;
        char const* argv[argc] = {
            __PRETTY_FUNCTION__,
            "--enable-num-framebuffers-quirk=false"
        };

        options.parse_arguments(desc, argc, argv);
    }

    void disable_single_gralloc_instance_quirk()
    {
        int const argc = 2;
        char const* argv[argc] = {
            __PRETTY_FUNCTION__,
            "--enable-single-gralloc-instance-quirk=false"
        };

        options.parse_arguments(desc, argc, argv);
    }

    void disable_width_alignment_quirk()
    {
        int const argc = 2;
        char const* argv[argc] = {
            __PRETTY_FUNCTION__,
            "--enable-width-alignment-quirk=false"
        };

        options.parse_arguments(desc, argc, argv);
    }

    bpo::options_description desc{"Options"};
    mir::options::ProgramOption options;
};

TEST_F(DeviceQuirks, number_of_framebuffers_quirk_can_be_disabled)
{
    using namespace testing;
    char const default_str[] = "";
    char const name_str[] = "mx3";

    MockOps mock_ops;
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device"), _, StrEq(default_str)))
        .Times(1)
        .WillOnce(Invoke([&]
        (char const*, char* value, char const*)
        {
            strncpy(value, name_str, PROP_VALUE_MAX);
            return 0;
        }));

    disable_num_framebuffers_quirk();
    mga::DeviceQuirks quirks(mock_ops, options);
    EXPECT_THAT(quirks.num_framebuffers(), Ne(3));
}

TEST_F(DeviceQuirks, single_gralloc_instance_quirk_can_be_disabled)
{
    using namespace testing;
    char const default_str[] = "";
    char const krillin_name_str[] = "krillin";

    MockOps mock_ops;
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device"), _, StrEq(default_str)))
        .Times(1)
        .WillOnce(Invoke([&](char const*, char* value, char const*)
        {
            strncpy(value, krillin_name_str, PROP_VALUE_MAX);
            return 0;
        }));

    disable_single_gralloc_instance_quirk();
    mga::DeviceQuirks quirks(mock_ops, options);
    EXPECT_TRUE(quirks.gralloc_reopenable_after_close());
}

TEST_F(DeviceQuirks, width_alignment_quirk_can_be_disabled)
{
    using namespace testing;
    char const default_str[] = "";
    char const vegetahd_name_str[] = "vegetahd";

    MockOps mock_ops;
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device"), _, StrEq(default_str)))
        .Times(1)
        .WillOnce(Invoke([&](char const*, char* value, char const*)
        {
            strncpy(value, vegetahd_name_str, PROP_VALUE_MAX);
            return 0;
        }));

    disable_width_alignment_quirk();
    mga::DeviceQuirks quirks(mock_ops, options);
    EXPECT_THAT(quirks.aligned_width(720), Eq(720));
}
