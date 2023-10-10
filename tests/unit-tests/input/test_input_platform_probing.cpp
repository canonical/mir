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

#include <gtest/gtest.h>

#include "mir/input/input_probe.h"

#include "src/server/report/null_report_factory.h"

#include "mir/options/option.h"
#include "mir/emergency_cleanup_registry.h"

#include "mir_test_framework/udev_environment.h"
#include "mir_test_framework/executable_path.h"
#include "mir_test_framework/stub_input_platform.h"
#include "mir/test/doubles/mock_x11.h"
#include "mir/test/doubles/mock_libinput.h"
#include "mir/test/doubles/mock_option.h"
#include "mir/test/doubles/mock_input_device_registry.h"
#include "src/platforms/evdev/platform.h"
#include "src/platforms/x11/input/input_platform.h"
#include "src/platforms/virtual/input/input_platform.h"
#include "mir/test/fake_shared.h"

namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mi = mir::input;
namespace mr = mir::report;
namespace mtf = mir_test_framework;

using namespace ::testing;

namespace
{
struct StubEmergencyCleanupRegistry : mir::EmergencyCleanupRegistry
{
    void add(mir::EmergencyCleanupHandler const&) override {}
    void add(mir::ModuleEmergencyCleanupHandler) override {}
};
char const platform_input_lib[] = "platform-input-lib";
char const platform_path[] = "platform-path";
char const platform_display_libs[] = "platform-display-libs";

struct InputPlatformProbe : ::testing::Test
{
    InputPlatformProbe()
    {
        ON_CALL(mock_options, is_set(StrEq(platform_input_lib))).WillByDefault(Return(false));
        ON_CALL(mock_options, is_set(StrEq(platform_path))).WillByDefault(Return(true));
        ON_CALL(mock_options, is_set(StrEq(platform_display_libs))).WillByDefault(Return(true));
        ON_CALL(mock_options, get(StrEq(platform_path),Matcher<char const*>(_)))
            .WillByDefault(Return(platform_path_value));
        ON_CALL(mock_options, get(StrEq(platform_path)))
            .WillByDefault(Invoke(
                    [this](char const*) -> boost::any const&
                    {
                        return platform_path_value_as_any;
                    }));
        ON_CALL(mock_options, get(StrEq(platform_input_lib)))
            .WillByDefault(Invoke(
                    [this](char const*) -> boost::any const&
                    {
                        return platform_input_lib_value_as_any;
                    }));
        ON_CALL(mock_options, get(StrEq(platform_display_libs_value)))
            .WillByDefault(Invoke(
                [this](char const*) -> boost::any const&
                {
                    return platform_display_libs_value_as_any;
                }));
#ifdef MIR_BUILD_PLATFORM_X11
        ON_CALL(mock_x11, XGetXCBConnection(_))
            .WillByDefault(Return(reinterpret_cast<xcb_connection_t*>(1)));
#endif
    }

    void disable_x11()
    {
#ifdef MIR_BUILD_PLATFORM_X11
        ON_CALL(mock_x11, XOpenDisplay(_)).WillByDefault(Return(nullptr));
#endif
    }

    // replace with with mocks for udev and evdev to simulate root or non-root
    // access on evdev devices, and enable the disabled test case(s) below.
    mtf::UdevEnvironment env;

#ifdef MIR_BUILD_PLATFORM_X11
    NiceMock<mtd::MockX11> mock_x11;
#endif
    NiceMock<mtd::MockLibInput> mock_libinput;
    NiceMock<mtd::MockOption> mock_options;
    mtd::MockInputDeviceRegistry mock_registry;
    StubEmergencyCleanupRegistry stub_emergency;
    std::shared_ptr<mir::SharedLibraryProberReport> stub_prober_report{mr::null_shared_library_prober_report()};
    std::string platform_path_value{mtf::server_platform_path()};
    boost::any platform_path_value_as_any{platform_path_value};
    std::string platform_input_lib_value;
    boost::any platform_input_lib_value_as_any;
    std::string platform_display_libs_value;
    boost::any platform_display_libs_value_as_any;
    std::vector<std::shared_ptr<mir::SharedLibrary>> const empty_loaded_module_list{};
};

template <typename Expected>
struct OfPtrTypeMatcher
{
    template <typename T>
    bool MatchAndExplain(T&& p, MatchResultListener* /* listener */) const
    {
        return dynamic_cast<Expected*>(&*p) != nullptr;
    }
    void DescribeTo(::std::ostream* os) const
    {
        *os << "is a or derived from a " << typeid(Expected).name();
    }
    void DescribeNegationTo(::std::ostream* os) const
    {
        *os << "is not or not derived from " << typeid(Expected).name();
    }
};

template<typename Dest>
inline auto OfPtrType()
{
    return MakePolymorphicMatcher(OfPtrTypeMatcher<Dest>());
}
}


TEST_F(InputPlatformProbe, stub_platform_not_picked_up_by_default)
{
    disable_x11();
    auto platform =
        mi::probe_input_platforms(
            mock_options,
            mt::fake_shared(stub_emergency),
            mt::fake_shared(mock_registry),
            nullptr,
            mr::null_input_report(),
            empty_loaded_module_list,
            *stub_prober_report);

    EXPECT_THAT(platform, OfPtrType<mi::virt::VirtualInputPlatform>());
}

#ifdef MIR_BUILD_PLATFORM_X11
char const vt[] = "vt";

TEST_F(InputPlatformProbe, x11_input_platform_not_used_when_vt_specified)
{
    ON_CALL(mock_options, is_set(StrEq(vt))).WillByDefault(Return(true));
    auto platform =
        mi::probe_input_platforms(
            mock_options,
            mt::fake_shared(stub_emergency),
            mt::fake_shared(mock_registry),
            nullptr,
            mr::null_input_report(),
            empty_loaded_module_list,
            *stub_prober_report);

    EXPECT_THAT(platform, OfPtrType<mi::virt::VirtualInputPlatform>());
}

#endif

TEST_F(InputPlatformProbe, allows_forcing_stub_input_platform)
{
    ON_CALL(mock_options, is_set(StrEq(platform_input_lib))).WillByDefault(Return(true));
    platform_input_lib_value = mtf::server_input_platform("input-stub.so");
    platform_input_lib_value_as_any = platform_input_lib_value;
    auto platform =
        mi::probe_input_platforms(
            mock_options,
            mt::fake_shared(stub_emergency),
            mt::fake_shared(mock_registry),
            nullptr,
            mr::null_input_report(),
            empty_loaded_module_list,
            *stub_prober_report);
    EXPECT_THAT(platform, OfPtrType<mtf::StubInputPlatform>());
}
