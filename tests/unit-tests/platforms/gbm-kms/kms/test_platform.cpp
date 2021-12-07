/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/event_handler_register.h"
#include "src/platforms/gbm-kms/server/kms/platform.h"
#include "src/platforms/gbm-kms/server/kms/quirks.h"
#include "src/server/report/null_report_factory.h"
#include "mir/shared_library.h"
#include "mir/options/program_option.h"

#include "mir/test/doubles/mock_buffer.h"
#include "mir/test/doubles/mock_console_services.h"
#include "mir/test/doubles/null_console_services.h"
#include "mir/test/doubles/null_emergency_cleanup.h"

#include <gtest/gtest.h>

#include "mir_test_framework/udev_environment.h"
#include "mir_test_framework/executable_path.h"
#include "mir_test_framework/temporary_environment_value.h"
#include "mir/test/pipe.h"

#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_gbm.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/fd_matcher.h"
#include "mir/test/doubles/stub_console_services.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <atomic>
#include <thread>
#include <chrono>

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{

const char rendering_platform_probe_symbol[] = "probe_rendering_platform";
const char display_platform_probe_symbol[] = "probe_display_platform";
const char add_platform_options_symbol[] = "add_graphics_platform_options";

class MesaGraphicsPlatform : public ::testing::Test
{
public:
    void SetUp()
    {
        using namespace testing;
        Mock::VerifyAndClearExpectations(&mock_drm);
        Mock::VerifyAndClearExpectations(&mock_gbm);
        ON_CALL(mock_egl, eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS))
            .WillByDefault(Return("EGL_AN_extension_string EGL_EXT_platform_base EGL_KHR_platform_gbm"));
        ON_CALL(mock_egl, eglGetDisplay(_))
            .WillByDefault(Return(fake_display));
        ON_CALL(mock_gl, glGetString(GL_RENDERER))
            .WillByDefault(
                Return(
                    reinterpret_cast<GLubyte const*>("GeForce GTX 1070/PCIe/SSE2")));
        ON_CALL(mock_egl, eglGetConfigAttrib(_, _, EGL_NATIVE_VISUAL_ID, _))
            .WillByDefault(
                DoAll(
                    SetArgPointee<3>(GBM_FORMAT_XRGB8888),
                    Return(EGL_TRUE)));
        fake_devices.add_standard_device("standard-drm-devices");
    }

    std::shared_ptr<mgg::Platform> create_platform()
    {
        return std::make_shared<mgg::Platform>(
                mir::report::null_display_report(),
                std::make_shared<mtd::StubConsoleServices>(),
                *std::make_shared<mtd::NullEmergencyCleanup>(),
                mgg::BypassOption::allowed,
                std::make_unique<mgg::Quirks>(mir::options::ProgramOption{}));
    }

    auto parsed_options_from_args(
        std::initializer_list<char const*> const& options,
        boost::program_options::options_description const& description) const
            -> std::unique_ptr<mir::options::ProgramOption>
    {
        auto parsed_options = std::make_unique<mir::options::ProgramOption>();

        auto argv = std::make_unique<char const*[]>(options.size() + 1);
        argv[0] = "argv0";
        std::copy(options.begin(), options.end(), &argv[1]);

        parsed_options->parse_arguments(description, static_cast<int>(options.size() + 1), argv.get());
        return parsed_options;
    }

    EGLDisplay fake_display{reinterpret_cast<EGLDisplay>(0xabcd)};
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockGL> mock_gl;
    mtf::UdevEnvironment fake_devices;
};
}

TEST_F(MesaGraphicsPlatform, a_failure_while_creating_a_platform_results_in_an_error)
{
    using namespace ::testing;

    EXPECT_CALL(mock_drm, open(_,_))
            .WillRepeatedly(SetErrnoAndReturn(EINVAL, -1));

    try
    {
        auto platform = create_platform();
    } catch(std::exception const&)
    {
        return;
    }

    FAIL() << "Expected an exception to be thrown.";
}

TEST_F(MesaGraphicsPlatform, display_probe_returns_unsupported_when_no_drm_udev_devices)
{
    mtf::UdevEnvironment udev_environment;
    mir::options::ProgramOption options;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(display_platform_probe_symbol);
    EXPECT_EQ(mg::PlatformPriority::unsupported, probe(stub_vt, options));
}

TEST_F(MesaGraphicsPlatform, rendering_probe_returns_unsupported_when_no_drm_udev_devices)
{
    mtf::UdevEnvironment udev_environment;
    mir::options::ProgramOption options;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(rendering_platform_probe_symbol);
    EXPECT_EQ(mg::PlatformPriority::unsupported, probe(stub_vt, options));
}

TEST_F(MesaGraphicsPlatform, display_probe_returns_best_when_master)
{
    mtf::UdevEnvironment udev_environment;
    boost::program_options::options_description po;
    mir::options::ProgramOption options;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    udev_environment.add_standard_device("standard-drm-devices");

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(display_platform_probe_symbol);
    EXPECT_EQ(mg::PlatformPriority::best, probe(stub_vt, options));
}

TEST_F(MesaGraphicsPlatform, rendering_probe_returns_supported_on_llvmpipe)
{
    mtf::UdevEnvironment udev_environment;
    boost::program_options::options_description po;
    mir::options::ProgramOption options;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    udev_environment.add_standard_device("standard-drm-devices");

    ON_CALL(mock_gl, glGetString(GL_RENDERER))
        .WillByDefault(
            testing::Return(
                reinterpret_cast<GLubyte const*>("llvmpipe (you know, some version)")));

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(rendering_platform_probe_symbol);
    EXPECT_EQ(mg::PlatformPriority::supported, probe(stub_vt, options));
}

TEST_F(MesaGraphicsPlatform, probe_returns_unsupported_when_egl_client_extensions_not_supported)
{
    using namespace testing;

    mtf::UdevEnvironment udev_environment;
    boost::program_options::options_description po;
    mir::options::ProgramOption options;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    udev_environment.add_standard_device("standard-drm-devices");

    ON_CALL(mock_egl, eglQueryString(EGL_NO_DISPLAY, _))
        .WillByDefault(Return(nullptr));

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(rendering_platform_probe_symbol);
    EXPECT_EQ(mg::PlatformPriority::unsupported, probe(stub_vt, options));
}

TEST_F(MesaGraphicsPlatform, probe_returns_supported_when_old_egl_mesa_gbm_platform_supported)
{
    using namespace testing;

    mtf::UdevEnvironment udev_environment;
    boost::program_options::options_description po;
    mir::options::ProgramOption options;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    udev_environment.add_standard_device("standard-drm-devices");

    ON_CALL(mock_egl, eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS))
        .WillByDefault(Return("EGL_KHR_not_really_an_extension EGL_MESA_platform_gbm EGL_EXT_master_of_the_house EGL_EXT_platform_base"));

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(rendering_platform_probe_symbol);
    EXPECT_EQ(mg::PlatformPriority::best, probe(stub_vt, options));
}

TEST_F(MesaGraphicsPlatform, probe_returns_unsupported_when_gbm_platform_not_supported)
{
    using namespace testing;

    mtf::UdevEnvironment udev_environment;
    boost::program_options::options_description po;
    mir::options::ProgramOption options;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    udev_environment.add_standard_device("standard-drm-devices");

    ON_CALL(mock_egl, eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS))
        .WillByDefault(Return("EGL_KHR_not_really_an_extension EGL_EXT_platform_base"));

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(rendering_platform_probe_symbol);
    EXPECT_EQ(mg::PlatformPriority::unsupported, probe(stub_vt, options));
}

TEST_F(MesaGraphicsPlatform, probe_returns_supported_when_EGL_supports_gbm_platform_but_fails_to_get_EGL_display)
{
    using namespace testing;

    mtf::UdevEnvironment udev_environment;
    boost::program_options::options_description po;
    mir::options::ProgramOption options;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    udev_environment.add_standard_device("standard-drm-devices");

    ON_CALL(mock_egl, eglGetPlatformDisplayEXT(_,_,_))
        .WillByDefault(Return(EGL_NO_DISPLAY));

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(rendering_platform_probe_symbol);
    EXPECT_EQ(mg::PlatformPriority::supported, probe(stub_vt, options));
}

TEST_F(MesaGraphicsPlatform, probe_returns_unsupported_when_modesetting_is_not_supported)
{
    using namespace testing;

    boost::program_options::options_description po;
    mir::options::ProgramOption options;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    ON_CALL(mock_drm, drmCheckModesettingSupported(_)).WillByDefault(Return(-ENOSYS));

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(display_platform_probe_symbol);
    EXPECT_EQ(mg::PlatformPriority::unsupported, probe(stub_vt, options));
}

TEST_F(MesaGraphicsPlatform, probe_returns_supported_when_cannot_determine_kms_support)
{
    using namespace testing;

    boost::program_options::options_description po;
    mir::options::ProgramOption options;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    ON_CALL(mock_drm, drmCheckModesettingSupported(_)).WillByDefault(Return(-EINVAL));

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(display_platform_probe_symbol);
    EXPECT_EQ(mg::PlatformPriority::supported, probe(stub_vt, options));

}

TEST_F(MesaGraphicsPlatform, probe_returns_supported_when_unexpected_error_returned)
{
    using namespace testing;

    boost::program_options::options_description po;
    mir::options::ProgramOption options;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    ON_CALL(mock_drm, drmCheckModesettingSupported(_)).WillByDefault(Return(-ENOBUFS));

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(display_platform_probe_symbol);
    EXPECT_EQ(mg::PlatformPriority::supported, probe(stub_vt, options));

}

TEST_F(MesaGraphicsPlatform, probe_returns_supported_when_cannot_determine_busid)
{
    using namespace testing;

    boost::program_options::options_description po;
    mir::options::ProgramOption options;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    ON_CALL(mock_drm, drmGetBusid(_)).WillByDefault(Return(nullptr));

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(display_platform_probe_symbol);
    EXPECT_EQ(mg::PlatformPriority::supported, probe(stub_vt, options));

}

TEST_F(MesaGraphicsPlatform, display_probe_returns_supported_when_KMS_probe_is_overridden)
{
    using namespace testing;

    boost::program_options::options_description po;
    mir::options::ProgramOption options;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    ON_CALL(mock_drm, drmCheckModesettingSupported(_)).WillByDefault(Return(-ENOSYS));
    mtf::TemporaryEnvironmentValue disable_kms_probe{"MIR_MESA_KMS_DISABLE_MODESET_PROBE", "1"};

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(display_platform_probe_symbol);
    EXPECT_EQ(mg::PlatformPriority::supported, probe(stub_vt, options));
}

TEST_F(MesaGraphicsPlatform, rendering_probe_succeeds_without_drm_master)
{
    using namespace testing;

    mtf::UdevEnvironment udev_environment;
    boost::program_options::options_description po;
    mir::options::ProgramOption options;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    udev_environment.add_standard_device("standard-drm-devices");
    ON_CALL(mock_drm, drmSetMaster(_))
        .WillByDefault(SetErrnoAndReturn(EPERM, -1));

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};
    auto probe = platform_lib.load_function<mg::PlatformProbe>(rendering_platform_probe_symbol);
    EXPECT_EQ(mg::PlatformPriority::best, probe(stub_vt, options));
}

TEST_F(MesaGraphicsPlatform, display_probe_does_not_touch_quirked_device)
{
    using namespace testing;

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};

    mtf::UdevEnvironment udev_environment;
    boost::program_options::options_description po;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    udev_environment.add_standard_device("standard-drm-devices");

    auto add_options = platform_lib.load_function<mg::AddPlatformOptions>(add_platform_options_symbol);

    add_options(po);
    /* NOTE: Implementation details of StubConsoleServices means we can only test the second device node
     *
     * Specifically: probe() will call StubConsoleServices::acquire_device(major, minor, _). However,
     * StubConsoleServices does not attempt to consult udev to do the (major, minor) → "/dev/dri/card?" mapping.
     * Instead, the *first* request for a DRM device gets "/dev/dri/card0", the second gets "/dev/dri/card1", etc.
     *
     * This means that even if the Quirks skip the first DRM device, StubConsoleServices will attempt to open
     * "/dev/dri/card0".
     *
     * This doesn't affect the quality of the test - we're already testing that Quirks handle things correctly.
     */
    auto const options_with_quirk = parsed_options_from_args({"--driver-quirks=skip:devnode:/dev/dri/card1"}, po);

    EXPECT_CALL(mock_drm, open(StrEq("/dev/dri/card0"), _))
        .Times(AtLeast(1));
    EXPECT_CALL(mock_drm, open(StrEq("/dev/dri/card1"), _))
        .Times(0);

    auto probe = platform_lib.load_function<mg::PlatformProbe>(display_platform_probe_symbol);
    probe(stub_vt, *options_with_quirk);
}

TEST_F(MesaGraphicsPlatform, render_probe_does_not_touch_quirked_device)
{
    using namespace testing;

    mir::SharedLibrary platform_lib{mtf::server_platform("graphics-gbm-kms")};

    mtf::UdevEnvironment udev_environment;
    boost::program_options::options_description po;
    auto const stub_vt = std::make_shared<mtd::StubConsoleServices>();

    udev_environment.add_standard_device("standard-drm-devices");

    auto add_options = platform_lib.load_function<mg::AddPlatformOptions>(add_platform_options_symbol);

    add_options(po);
    /* NOTE: Implementation details of StubConsoleServices means we can only test the second device node
     *
     * Specifically: probe() will call StubConsoleServices::acquire_device(major, minor, _). However,
     * StubConsoleServices does not attempt to consult udev to do the (major, minor) → "/dev/dri/card?" mapping.
     * Instead, the *first* request for a DRM device gets "/dev/dri/card0", the second gets "/dev/dri/card1", etc.
     *
     * This means that even if the Quirks skip the first DRM device, StubConsoleServices will attempt to open
     * "/dev/dri/card0".
     *
     * This doesn't affect the quality of the test - we're already testing that Quirks handle things correctly.
     */
    auto const options_with_quirk = parsed_options_from_args({"--driver-quirks=skip:devnode:/dev/dri/card1"}, po);

    EXPECT_CALL(mock_drm, open(StrEq("/dev/dri/card0"), _))
        .Times(AtLeast(1));
    EXPECT_CALL(mock_drm, open(StrEq("/dev/dri/card1"), _))
        .Times(0);

    auto probe = platform_lib.load_function<mg::PlatformProbe>(rendering_platform_probe_symbol);
    probe(stub_vt, *options_with_quirk);
}
