/*
 * Copyright © 2015 Canonical Ltd.
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

#include "mir/graphics/display.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/platform.h"
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
#include "src/platforms/mesa/server/kms/platform.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fcntl.h>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
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
        auto const platform = std::make_shared<mgm::Platform>(
                mir::report::null_display_report(),
                std::make_shared<mtd::StubConsoleServices>(),
                *std::make_shared<mtd::NullEmergencyCleanup>(),
                mgm::BypassOption::allowed);
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
