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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/shared_library.h"
#include "mir/input/platform.h"
#include "mir/console_services.h"
#include "mir/fd.h"

#include "mir/test/doubles/mock_x11.h"
#include "mir/test/doubles/mock_option.h"
#include "mir_test_framework/executable_path.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;
namespace mo = mir::options;
using namespace ::testing;
namespace
{
class StubConsoleServices : public mir::ConsoleServices
{
public:
    void
    register_switch_handlers(
        mir::graphics::EventHandlerRegister&,
        std::function<bool()> const&,
        std::function<bool()> const&) override
    {
    }

    void restore() override
    {
    }

    std::future<std::unique_ptr<mir::Device>> acquire_device(
        int /*major*/, int /*minor*/,
        mir::Device::OnDeviceActivated const& on_activated,
        mir::Device::OnDeviceSuspended const&,
        mir::Device::OnDeviceRemoved const&) override
    {
        std::promise<std::unique_ptr<mir::Device>> promise;
        on_activated(mir::Fd{mir::IntOwnedFd{42}});
        promise.set_value(nullptr);

        return promise.get_future();
    }
};

auto get_x11_platform()
{
    auto path = mtf::server_platform("server-mesa-x11");
    return std::make_shared<mir::SharedLibrary>(path);
}

char const probe_input_platform_symbol[] = "probe_input_platform";
char const host_socket_opt[] = "host-socket";

struct X11Platform : Test
{

    NiceMock<mtd::MockOption> options;
    NiceMock<mtd::MockX11> mock_x11;
    std::shared_ptr<mir::SharedLibrary> library{get_x11_platform()};
    std::shared_ptr<StubConsoleServices> const console{std::make_shared<StubConsoleServices>()};
};

}

TEST_F(X11Platform, probes_as_unsupported_without_display)
{
    ON_CALL(mock_x11, XOpenDisplay(_))
        .WillByDefault(Return(nullptr));

    auto probe_fun = library->load_function<mir::input::ProbePlatform>(probe_input_platform_symbol);
    EXPECT_THAT(probe_fun(options, *console), Eq(mir::input::PlatformPriority::unsupported));
}

TEST_F(X11Platform, probes_as_supported_with_display)
{
    // default setup of MockX11 already provides fake objects

    auto probe_fun = library->load_function<mir::input::ProbePlatform>(probe_input_platform_symbol);
    EXPECT_THAT(probe_fun(options, *console), Ge(mir::input::PlatformPriority::supported));
}

TEST_F(X11Platform, probes_as_unsupported_on_nested_configs)
{
    ON_CALL(options,is_set(StrEq(host_socket_opt)))
        .WillByDefault(Return(true));
    ON_CALL(options,get(StrEq(host_socket_opt), Matcher<char const*>(_)))
        .WillByDefault(Return("something"));

    auto probe_fun = library->load_function<mir::input::ProbePlatform>(probe_input_platform_symbol);
    EXPECT_THAT(probe_fun(options, *console), Eq(mir::input::PlatformPriority::unsupported));
}

