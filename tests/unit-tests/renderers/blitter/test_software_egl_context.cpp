/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "src/platform/renderers/blitter/software_egl_context.h"

#include <mir/test/doubles/mock_egl.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mrb = mir::renderer::blitter;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
auto const device_display{reinterpret_cast<EGLDisplay>(0xde71ce)};
auto const surfaceless_display{reinterpret_cast<EGLDisplay>(0x5e7f)};
auto const hardware_device{reinterpret_cast<EGLDeviceEXT>(0x4a7d)};
auto const software_device{reinterpret_cast<EGLDeviceEXT>(0x50f7)};
auto const other_context{reinterpret_cast<EGLContext>(0x07e4)};

std::vector<std::string> const required_display_extensions{
    "EGL_KHR_no_config_context",
    "EGL_KHR_surfaceless_context",
    "EGL_EXT_image_dma_buf_import"};

auto join(std::vector<std::string> const& extensions) -> std::string
{
    std::string result;
    for (auto const& extension : extensions)
    {
        result += extension + " ";
    }
    return result;
}

class SoftwareEGLContextTest : public Test
{
public:
    SoftwareEGLContextTest()
    {
        ON_CALL(mock_egl, eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS))
            .WillByDefault([this](auto, auto) { return client_extensions.c_str(); });
        ON_CALL(mock_egl, eglQueryString(Ne(EGL_NO_DISPLAY), EGL_EXTENSIONS))
            .WillByDefault([this](auto, auto) { return display_extensions.c_str(); });
        ON_CALL(mock_egl, eglGetPlatformDisplayEXT(EGL_PLATFORM_SURFACELESS_MESA, _, _))
            .WillByDefault(Return(surfaceless_display));
        ON_CALL(mock_egl, eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, _, _))
            .WillByDefault(Return(device_display));
    }

    void provide_devices(std::vector<std::pair<EGLDeviceEXT, char const*>> const& devices)
    {
        ON_CALL(mock_egl, eglQueryDevicesEXT(_, _, _))
            .WillByDefault(
                [devices](EGLint max_devices, EGLDeviceEXT* out, EGLint* num_devices)
                {
                    auto const count = static_cast<EGLint>(devices.size());
                    if (!out)
                    {
                        *num_devices = count;
                        return EGL_TRUE;
                    }
                    *num_devices = std::min(max_devices, count);
                    for (auto i = 0; i < *num_devices; ++i)
                    {
                        out[i] = devices[i].first;
                    }
                    return EGL_TRUE;
                });
        for (auto const& [device, extensions] : devices)
        {
            ON_CALL(mock_egl, eglQueryDeviceStringEXT(device, EGL_EXTENSIONS)).WillByDefault(Return(extensions));
        }
    }

    std::string client_extensions{"EGL_EXT_platform_base EGL_MESA_platform_surfaceless EGL_EXT_device_base"};
    std::string display_extensions{join(required_display_extensions) + "EGL_KHR_image_base"};
    NiceMock<mtd::MockEGL> mock_egl;
};
}

TEST_F(SoftwareEGLContextTest, throws_without_platform_base_extension)
{
    client_extensions = "EGL_MESA_platform_surfaceless EGL_EXT_device_base";

    EXPECT_THROW(mrb::SoftwareEGLContext{}, std::runtime_error);
}

TEST_F(SoftwareEGLContextTest, uses_software_device_when_available)
{
    provide_devices({{hardware_device, "EGL_EXT_device_drm"}, {software_device, "EGL_MESA_device_software"}});

    EXPECT_CALL(mock_egl, eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, software_device, _));
    EXPECT_CALL(mock_egl, eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, hardware_device, _)).Times(0);

    mrb::SoftwareEGLContext const context;

    EXPECT_THAT(context.display(), Eq(device_display));
}

TEST_F(SoftwareEGLContextTest, uses_software_device_with_separate_enumeration_and_query_extensions)
{
    client_extensions =
        "EGL_EXT_platform_base EGL_MESA_platform_surfaceless EGL_EXT_device_enumeration EGL_EXT_device_query";
    provide_devices({{software_device, "EGL_MESA_device_software"}});

    mrb::SoftwareEGLContext const context;

    EXPECT_THAT(context.display(), Eq(device_display));
}

TEST_F(SoftwareEGLContextTest, falls_back_to_surfaceless_when_no_device_is_software)
{
    provide_devices({{hardware_device, "EGL_EXT_device_drm"}});

    EXPECT_CALL(mock_egl, eglGetPlatformDisplayEXT(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(mock_egl, eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, _, _)).Times(0);

    mrb::SoftwareEGLContext const context;

    EXPECT_THAT(context.display(), Eq(surfaceless_display));
}

TEST_F(SoftwareEGLContextTest, falls_back_to_surfaceless_when_there_are_no_devices)
{
    mrb::SoftwareEGLContext const context;

    EXPECT_THAT(context.display(), Eq(surfaceless_display));
}

TEST_F(SoftwareEGLContextTest, falls_back_to_surfaceless_without_device_extensions)
{
    client_extensions = "EGL_EXT_platform_base EGL_MESA_platform_surfaceless";
    provide_devices({{software_device, "EGL_MESA_device_software"}});

    EXPECT_CALL(mock_egl, eglQueryDevicesEXT(_, _, _)).Times(0);

    mrb::SoftwareEGLContext const context;

    EXPECT_THAT(context.display(), Eq(surfaceless_display));
}

TEST_F(SoftwareEGLContextTest, falls_back_to_surfaceless_with_device_enumeration_but_not_device_query)
{
    client_extensions = "EGL_EXT_platform_base EGL_MESA_platform_surfaceless EGL_EXT_device_enumeration";
    provide_devices({{software_device, "EGL_MESA_device_software"}});

    std::unique_ptr<mrb::SoftwareEGLContext> context;
    ASSERT_NO_THROW(context = std::make_unique<mrb::SoftwareEGLContext>());

    EXPECT_THAT(context->display(), Eq(surfaceless_display));
}

TEST_F(SoftwareEGLContextTest, falls_back_to_surfaceless_when_device_entrypoints_are_unavailable)
{
    provide_devices({{software_device, "EGL_MESA_device_software"}});
    ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglQueryDevicesEXT"))).WillByDefault(Return(nullptr));

    mrb::SoftwareEGLContext const context;

    EXPECT_THAT(context.display(), Eq(surfaceless_display));
}

TEST_F(SoftwareEGLContextTest, falls_back_to_surfaceless_when_device_query_fails)
{
    provide_devices({{software_device, "EGL_MESA_device_software"}});
    ON_CALL(mock_egl, eglQueryDevicesEXT(_, _, _)).WillByDefault(Return(EGL_FALSE));

    mrb::SoftwareEGLContext const context;

    EXPECT_THAT(context.display(), Eq(surfaceless_display));
}

TEST_F(SoftwareEGLContextTest, falls_back_to_surfaceless_when_software_device_has_no_display)
{
    provide_devices({{software_device, "EGL_MESA_device_software"}});
    ON_CALL(mock_egl, eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, _, _)).WillByDefault(Return(EGL_NO_DISPLAY));

    mrb::SoftwareEGLContext const context;

    EXPECT_THAT(context.display(), Eq(surfaceless_display));
}

TEST_F(SoftwareEGLContextTest, throws_without_software_device_or_surfaceless_platform)
{
    client_extensions = "EGL_EXT_platform_base EGL_EXT_device_base";

    EXPECT_THROW(mrb::SoftwareEGLContext{}, std::runtime_error);
}

TEST_F(SoftwareEGLContextTest, throws_when_surfaceless_display_is_unavailable)
{
    ON_CALL(mock_egl, eglGetPlatformDisplayEXT(EGL_PLATFORM_SURFACELESS_MESA, _, _))
        .WillByDefault(Return(EGL_NO_DISPLAY));

    EXPECT_THROW(mrb::SoftwareEGLContext{}, std::runtime_error);
}

TEST_F(SoftwareEGLContextTest, throws_when_display_cannot_be_initialised)
{
    ON_CALL(mock_egl, eglInitialize(_, _, _)).WillByDefault(Return(EGL_FALSE));

    EXPECT_CALL(mock_egl, eglCreateContext(_, _, _, _)).Times(0);

    EXPECT_THROW(mrb::SoftwareEGLContext{}, std::runtime_error);
}

TEST_F(SoftwareEGLContextTest, creates_configless_gles2_context_on_selected_display)
{
    EXPECT_CALL(mock_egl, eglBindApi(EGL_OPENGL_ES_API)).Times(AtLeast(1));
    EXPECT_CALL(
        mock_egl,
        eglCreateContext(
            surfaceless_display,
            EGL_NO_CONFIG_KHR,
            EGL_NO_CONTEXT,
            mtd::EGLConfigContainsAttrib(EGL_CONTEXT_CLIENT_VERSION, 2)));

    mrb::SoftwareEGLContext const context;
}

TEST_F(SoftwareEGLContextTest, terminates_display_when_context_creation_fails)
{
    ON_CALL(mock_egl, eglCreateContext(_, _, _, _)).WillByDefault(Return(EGL_NO_CONTEXT));

    EXPECT_CALL(mock_egl, eglTerminate(surfaceless_display));

    EXPECT_THROW(mrb::SoftwareEGLContext{}, std::runtime_error);
}

TEST_F(SoftwareEGLContextTest, is_current_after_construction)
{
    EXPECT_CALL(
        mock_egl,
        eglMakeCurrent(surfaceless_display, EGL_NO_SURFACE, EGL_NO_SURFACE, mock_egl.fake_egl_context));

    mrb::SoftwareEGLContext const context;

    EXPECT_THAT(eglGetCurrentContext(), Eq(mock_egl.fake_egl_context));

    // The context's destructor also calls eglMakeCurrent()
    Mock::VerifyAndClearExpectations(&mock_egl);
}

TEST_F(SoftwareEGLContextTest, cleans_up_when_context_cannot_be_made_current)
{
    ON_CALL(mock_egl, eglMakeCurrent(_, _, _, mock_egl.fake_egl_context)).WillByDefault(Return(EGL_FALSE));

    EXPECT_CALL(mock_egl, eglDestroyContext(surfaceless_display, mock_egl.fake_egl_context));
    EXPECT_CALL(mock_egl, eglTerminate(surfaceless_display));

    EXPECT_THROW(mrb::SoftwareEGLContext{}, std::runtime_error);
}

TEST_F(SoftwareEGLContextTest, make_current_does_nothing_when_already_current)
{
    mrb::SoftwareEGLContext const context;

    EXPECT_CALL(mock_egl, eglMakeCurrent(_, _, _, _)).Times(0);

    context.make_current();

    // The context's destructor also calls eglMakeCurrent()
    Mock::VerifyAndClearExpectations(&mock_egl);
}

TEST_F(SoftwareEGLContextTest, make_current_makes_context_current_when_another_is)
{
    mrb::SoftwareEGLContext const context;
    eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, other_context);

    EXPECT_CALL(mock_egl, eglBindApi(EGL_OPENGL_ES_API));
    EXPECT_CALL(
        mock_egl,
        eglMakeCurrent(surfaceless_display, EGL_NO_SURFACE, EGL_NO_SURFACE, mock_egl.fake_egl_context));

    context.make_current();

    EXPECT_THAT(eglGetCurrentContext(), Eq(mock_egl.fake_egl_context));

    // The context's destructor also calls eglMakeCurrent()
    Mock::VerifyAndClearExpectations(&mock_egl);
}

TEST_F(SoftwareEGLContextTest, make_current_throws_on_failure)
{
    mrb::SoftwareEGLContext const context;
    eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, other_context);

    ON_CALL(mock_egl, eglMakeCurrent(_, _, _, mock_egl.fake_egl_context)).WillByDefault(Return(EGL_FALSE));

    EXPECT_THROW(context.make_current(), std::runtime_error);
}

TEST_F(SoftwareEGLContextTest, release_current_releases_context_when_current)
{
    mrb::SoftwareEGLContext const context;

    EXPECT_CALL(mock_egl, eglMakeCurrent(surfaceless_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));

    context.release_current();

    EXPECT_THAT(eglGetCurrentContext(), Eq(EGL_NO_CONTEXT));

    // The context's destructor also calls eglMakeCurrent()
    Mock::VerifyAndClearExpectations(&mock_egl);
}

TEST_F(SoftwareEGLContextTest, release_current_does_nothing_when_another_context_is_current)
{
    mrb::SoftwareEGLContext const context;
    eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, other_context);

    EXPECT_CALL(mock_egl, eglMakeCurrent(_, _, _, _)).Times(0);

    context.release_current();

    EXPECT_THAT(eglGetCurrentContext(), Eq(other_context));

    // The context's destructor also calls eglMakeCurrent()
    Mock::VerifyAndClearExpectations(&mock_egl);
}

TEST_F(SoftwareEGLContextTest, release_current_throws_on_failure)
{
    mrb::SoftwareEGLContext const context;

    ON_CALL(mock_egl, eglMakeCurrent(_, _, _, EGL_NO_CONTEXT)).WillByDefault(Return(EGL_FALSE));

    EXPECT_THROW(context.release_current(), std::runtime_error);
}

TEST_F(SoftwareEGLContextTest, destruction_releases_and_destroys_context_then_terminates_display)
{
    auto context = std::make_unique<mrb::SoftwareEGLContext>();

    InSequence seq;
    EXPECT_CALL(mock_egl, eglMakeCurrent(surfaceless_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
    EXPECT_CALL(mock_egl, eglDestroyContext(surfaceless_display, mock_egl.fake_egl_context));
    EXPECT_CALL(mock_egl, eglTerminate(surfaceless_display));

    context.reset();
}

class SoftwareEGLContextMissingDisplayExtension :
    public SoftwareEGLContextTest,
    public WithParamInterface<std::string>
{
};

TEST_P(SoftwareEGLContextMissingDisplayExtension, throws_and_terminates_display)
{
    auto extensions = required_display_extensions;
    std::erase(extensions, GetParam());
    display_extensions = join(extensions);

    EXPECT_CALL(mock_egl, eglTerminate(surfaceless_display));
    EXPECT_CALL(mock_egl, eglCreateContext(_, _, _, _)).Times(0);

    EXPECT_THROW(mrb::SoftwareEGLContext{}, std::runtime_error);
}

INSTANTIATE_TEST_SUITE_P(
    SoftwareEGLContextTest,
    SoftwareEGLContextMissingDisplayExtension,
    ValuesIn(required_display_extensions));
