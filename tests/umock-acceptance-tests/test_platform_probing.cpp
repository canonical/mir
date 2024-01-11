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

#include "mir/shared_library.h"
#include "mir_test_framework/udev_environment.h"
#include "mir/test/doubles/stub_console_services.h"
#include "mir/logging/null_shared_library_prober_report.h"
#include "mir/options/default_configuration.h"
#include "mir_test_framework/temporary_environment_value.h"
#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_gbm.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mo = mir::options;

class FullProbeStack : public testing::Test
{
public:
    FullProbeStack()
        : console{std::make_shared<mtd::StubConsoleServices>()},
          options{1, &"Platform Probing Acceptance Test"},
          report{std::make_shared<mir::logging::NullSharedLibraryProberReport>()}
    {
    }
    
    auto add_gbm_kms_device() -> bool
    {
        using namespace std::string_literals;
#ifndef MIR_BUILD_PLATFORM_GBM_KMS
        return false;
#else
       udev_env.add_device("drm", ("dri/card" + std::to_string(drm_device_count)).c_str(), nullptr, {}, {});
       drm_device_count++;
       return true;
#endif
    }

    void enable_host_x11()
    {
        temporary_env.emplace_back("DISPLAY", ":0");
    }

    void disable_host_x11()
    {
        temporary_env.emplace_back("DISPLAY", nullptr);
    }

    void enable_host_wayland()
    {
        temporary_env.emplace_back("wayland-host", "WAYLAND-0");
    }

    auto the_options() -> mir::options::Option const&
    {
        return options;
    }

    auto the_console_services() -> std::shared_ptr<mir::ConsoleServices>
    {
        return console;
    }

    auto the_library_prober_report() -> std::shared_ptr<mir::SharedLibraryProberReport>
    {
        return report;
    }
private:
    std::shared_ptr<mir::ConsoleServices> const console;
    mir::options::DefaultConfiguration options;
    std::shared_ptr<mir::SharedLibraryProberReport> const report;
    
    std::vector<mtf::TemporaryEnvironmentValue> temporary_env;
    mtf::UdevEnvironment udev_env;
#ifdef MIR_BUILD_PLATFORM_GBM_KMS
    int drm_device_count{0};
    mtd::MockDRM drm;
    mtd::MockGBM gbm;
#endif
};

TEST_F(FullProbeStack, select_display_modules_loads_all_available_hardware_when_no_nested)
{
    using namespace testing;
    int expected_hardware_count{0};
    for (auto i = 0; i < 2; ++i)
    {
        if (add_gbm_kms_device())
        {
            expected_hardware_count++;
        }
    }

    auto devices = mg::select_display_modules(the_options(), the_console_services(), *the_library_prober_report());
    EXPECT_THAT(devices.size(), Eq(expected_hardware_count));    
}


