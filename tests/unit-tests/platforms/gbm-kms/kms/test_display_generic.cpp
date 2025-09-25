/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/graphics/display.h"
#include "mir/graphics/display_configuration.h"
#include "mir/options/program_option.h"

#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/stub_gl_config.h"
#include "mir/test/doubles/null_emergency_cleanup.h"
#include "mir/test/doubles/stub_console_services.h"
#include "src/server/report/null_report_factory.h"
#include "mir/graphics/default_display_configuration_policy.h"
#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_gbm.h"
#include "mir_test_framework/udev_environment.h"
#include "src/platforms/gbm-kms/server/kms/platform.h"
#include "src/platforms/gbm-kms/server/kms/quirks.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fcntl.h>

namespace mg = mir::graphics;
namespace mgg = mg::gbm;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

class DisplayTestGeneric : public ::testing::Test
{
public:
    DisplayTestGeneric()
        : drm_fd{open(drm_device, 0, 0)}
    {
        using namespace testing;

        ON_CALL(mock_egl, eglChooseConfig(_,_,_,1,_))
            .WillByDefault(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                                 SetArgPointee<4>(1),
                                 Return(EGL_TRUE)));
        ON_CALL(mock_egl, eglGetConfigAttrib(_, mock_egl.fake_configs[0], EGL_NATIVE_VISUAL_ID, _))
            .WillByDefault(
                DoAll(
                    SetArgPointee<3>(GBM_FORMAT_XRGB8888),
                    Return(EGL_TRUE)));

        // We sometimes want to copy the config of an existing context.
        // Since our mocks don't check what config attribs we're asking for,
        // we can just make something up.
        ON_CALL(mock_egl, eglQueryContext(_,_,EGL_CONFIG_ID,_))
            .WillByDefault(
                DoAll(
                    SetArgPointee<3>(0xabadbaab),
                    Return(EGL_TRUE)));

        mock_egl.provide_egl_extensions();
        mock_gl.provide_gles_extensions();

        ON_CALL(mock_gbm, gbm_device_get_fd(_))
            .WillByDefault(Return(drm_fd));

        fake_devices.add_standard_device("standard-drm-devices");

        // Remove all outputs from all but one of the DRM devices we access;
        // these tests are not set up to test hybrid.
        mock_drm.reset("/dev/dri/card1");
        mock_drm.reset("/dev/dri/card2");
    }

    std::shared_ptr<mg::Display> create_display()
    {
        mir::udev::Context ctx;
        // Caution: non-local state!
        // This works because standard-drm-devices contains a udev device with 226:0 and devnode /dev/dri/card0
        auto device = ctx.char_device_from_devnum(makedev(226, 0));

        auto platform = std::make_shared<mgg::Platform>(
            *device,
            mir::report::null_display_report(),
            *std::make_shared<mtd::StubConsoleServices>(),
            *std::make_shared<mtd::NullEmergencyCleanup>(),
            mgg::BypassOption::allowed);
        return platform->create_display(
            std::make_shared<mg::CloneDisplayConfigurationPolicy>(),
            std::make_shared<mtd::StubGLConfig>());
    }

    ::testing::NiceMock<mtd::MockEGL> mock_egl;
    ::testing::NiceMock<mtd::MockGL> mock_gl;
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    mtf::UdevEnvironment fake_devices;

    char const* const drm_device = "/dev/dri/card0";
    int const drm_fd;
};

#include "../../test_display.h"
