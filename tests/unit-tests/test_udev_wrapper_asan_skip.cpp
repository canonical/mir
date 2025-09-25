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

#include "mir_test_framework/udev_environment.h"
#include "mir/udev/wrapper.h"
#include "mir/test/death.h"

#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <libudev.h>

namespace mtf=mir_test_framework;

namespace
{
bool KilledByInvalidMemoryAccess(int exit_status)
{
    return testing::KilledBySignal(SIGSEGV)(exit_status) ||
           testing::KilledBySignal(SIGBUS)(exit_status) ||
           testing::KilledBySignal(SIGABRT)(exit_status) ||
           // It seems that valgrind kills us with SIGKILL
           testing::KilledBySignal(SIGKILL)(exit_status);
}

/// Tests in this suite are skipped during the address sanitizer build,
/// as they purposefully do things that the address sanitizer would not like.
class UdevWrapperDeathTest : public ::testing::Test
{
public:
    mtf::UdevEnvironment udev_environment;
};
}

TEST_F(UdevWrapperDeathTest, DereferencingEndReturnsInvalidObject)
{
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    udev_environment.add_device("drm", "control64D", NULL, {}, {});
    udev_environment.add_device("drm", "card1", NULL, {}, {});

    mir::udev::Enumerator devices(std::make_shared<mir::udev::Context>());

    devices.scan_devices();

    MIR_EXPECT_EXIT((*devices.end()).subsystem(), KilledByInvalidMemoryAccess, "");

    auto iter = devices.begin();

    while(iter != devices.end())
    {
        iter++;
    }
    MIR_EXPECT_EXIT((*iter).subsystem(), KilledByInvalidMemoryAccess, "");
}

TEST_F(UdevWrapperDeathTest, MemberDereferenceOfEndDies)
{
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    udev_environment.add_device("drm", "control64D", NULL, {}, {});
    udev_environment.add_device("drm", "card1", NULL, {}, {});

    mir::udev::Enumerator devices(std::make_shared<mir::udev::Context>());

    devices.scan_devices();

    MIR_EXPECT_EXIT(devices.end()->subsystem(), KilledByInvalidMemoryAccess, "");

    auto iter = devices.begin();

    while(iter != devices.end())
    {
        iter++;
    }
    MIR_EXPECT_EXIT(iter->subsystem(), KilledByInvalidMemoryAccess, "");
}
