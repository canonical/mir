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
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/mock_egl.h"
#include <hardware/gralloc.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mga = mir::graphics::android;
namespace bpo = boost::program_options;
namespace mtd = mir::test::doubles;

namespace
{
struct MockOps : mga::PropertiesWrapper
{
    MOCK_CONST_METHOD3(property_get, int(
        char const[PROP_NAME_MAX], char[PROP_VALUE_MAX], char const[PROP_VALUE_MAX]));
};
}

struct DeviceDetection : testing::Test
{
    testing::NiceMock<mtd::MockEGL> mock_egl;
    testing::NiceMock<mtd::MockGL> mock_gl;
};

TEST_F(DeviceDetection, two_buffers_by_default)
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

TEST_F(DeviceDetection, three_buffers_reported_for_mx3)
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
TEST_F(DeviceDetection, reports_gralloc_can_be_closed_safely_by_default)
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
    EXPECT_FALSE(quirks.gralloc_cannot_be_closed_safely());
}

TEST_F(DeviceDetection, reports_gralloc_cannot_be_closed_safely_on_krillin)
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
    EXPECT_TRUE(quirks.gralloc_cannot_be_closed_safely());
}

TEST_F(DeviceDetection, aligns_width_on_vegetahd)
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

TEST_F(DeviceDetection, clears_gl_context_fence_on_manta)
{
    using namespace testing;
    char const default_str[] = "";
    char const manta_name_str[] = "manta";

    MockOps mock_ops;
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device"), _, StrEq(default_str)))
        .Times(1)
        .WillOnce(Invoke([&](char const*, char* value, char const*)
        {
            strncpy(value, manta_name_str, PROP_VALUE_MAX);
            return 0;
        }));

    mga::DeviceQuirks quirks(mock_ops);
    EXPECT_TRUE(quirks.clear_fb_context_fence());
}

TEST_F(DeviceDetection, clears_gl_context_fence_on_arale)
{
    using namespace testing;
    char const default_str[] = "";
    char const arale_name_str[] = "arale";

    MockOps mock_ops;
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device"), _, StrEq(default_str)))
        .Times(1)
        .WillOnce(Invoke([&](char const*, char* value, char const*)
        {
            strncpy(value, arale_name_str, PROP_VALUE_MAX);
            return 0;
        }));

    mga::DeviceQuirks quirks(mock_ops);
    EXPECT_TRUE(quirks.clear_fb_context_fence());
}

TEST_F(DeviceDetection, clears_gl_context_fence_on_krillin)
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
    EXPECT_TRUE(quirks.clear_fb_context_fence());
}

TEST_F(DeviceDetection, does_not_clear_gl_context_fence_on_others)
{
    using namespace testing;
    char const default_str[] = "";
    char const other_name_str[] = "others";

    MockOps mock_ops;
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device"), _, StrEq(default_str)))
        .Times(1)
        .WillOnce(Invoke([&](char const*, char* value, char const*)
        {
            strncpy(value, other_name_str, PROP_VALUE_MAX);
            return 0;
        }));

    mga::DeviceQuirks quirks(mock_ops);
    EXPECT_FALSE(quirks.clear_fb_context_fence());
}

struct DeviceQuirks : testing::Test
{
    void SetUp()
    {
        mga::DeviceQuirks::add_options(desc);
    }

    void enable_fb_ion_quirk()
    {
        int const argc = 2;
        char const* argv[argc] = {
            __PRETTY_FUNCTION__,
            "--fb-ion-heap=false"
        };

        options.parse_arguments(desc, argc, argv);
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

    void disable_gralloc_cannot_be_closed_safely_quirk()
    {
        int const argc = 2;
        char const* argv[argc] = {
            __PRETTY_FUNCTION__,
            "--enable-gralloc-cannot-be-closed-safely-quirk=false"
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
    testing::NiceMock<mtd::MockGL> mock_gl;
    testing::NiceMock<mtd::MockEGL> mock_egl;
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

TEST_F(DeviceQuirks, gralloc_cannot_be_closed_safely_quirk_can_be_disabled)
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

    disable_gralloc_cannot_be_closed_safely_quirk();
    mga::DeviceQuirks quirks(mock_ops, options);
    EXPECT_FALSE(quirks.gralloc_cannot_be_closed_safely());
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

TEST_F(DeviceQuirks, returns_correct_gralloc_bits_with_fb_ion_quirk_for_device)
{
    using namespace testing;
    char const default_str[] = "";
    char const name_str[] = "Aquaris_M10_FHD";
    MockOps mock_ops;
    EXPECT_CALL(mock_ops, property_get(StrEq("ro.product.device"), _, StrEq(default_str)))
        .Times(1)
        .WillOnce(Invoke([&](char const*, char* value, char const*)
        {
            strncpy(value, name_str, PROP_VALUE_MAX);
            return 0;
        }));

    mga::DeviceQuirks quirks(mock_ops, options);
    EXPECT_THAT(quirks.fb_gralloc_bits(),
        testing::Eq(GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_TEXTURE));
}

TEST_F(DeviceQuirks, returns_correct_gralloc_bits_without_fb_ion_quirk)
{
    MockOps mock_ops;
    mga::DeviceQuirks quirks(mock_ops, options);
    EXPECT_THAT(quirks.fb_gralloc_bits(),
        testing::Eq(GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_FB));
}

TEST_F(DeviceQuirks, returns_correct_gralloc_bits_with_fb_ion_quirk)
{
    using namespace testing;
    MockOps mock_ops;
    enable_fb_ion_quirk();
    mga::DeviceQuirks quirks(mock_ops, options);
    EXPECT_THAT(quirks.fb_gralloc_bits(),
        testing::Eq(GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_TEXTURE));
}

TEST_F(DeviceQuirks, detects_gl_vendor_and_renderer_for_egl_quirk)
{
    using namespace testing;
    EXPECT_CALL(mock_gl, glGetString(GL_VENDOR))
        .Times(2)
        .WillOnce(Return(reinterpret_cast<const unsigned char*>("Qualcomm")))
        .WillOnce(Return(reinterpret_cast<const unsigned char*>("gpuMart")));
    EXPECT_CALL(mock_gl, glGetString(GL_RENDERER))
        .Times(2);
    MockOps mock_ops;
    mga::DeviceQuirks quirks(mock_ops, options);
    EXPECT_TRUE(quirks.working_egl_sync());

    mga::DeviceQuirks broken_quirks(mock_ops, options);
    EXPECT_FALSE(broken_quirks.working_egl_sync());
}
