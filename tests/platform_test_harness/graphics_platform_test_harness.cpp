/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include "mir/graphics/platform.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_sink.h"
#include "mir/main_loop.h"
#include "mir/options/program_option.h"
#include "mir/shared_library.h"
#include "mir/udev/wrapper.h"
#include "mir/default_server_configuration.h"
#include "mir/emergency_cleanup.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <boost/exception/diagnostic_information.hpp>
#include <chrono>
#include <iostream>
#include <mir/geometry/displacement.h>
#include <thread>
#include <unistd.h>

namespace mg = mir::graphics;

namespace
{
//TODO: With a little bit of reworking of the CMake files it would be possible to
//just depend on the relevant object libraries and not pull in the whole DefaultServerConfiguration
class MinimalServerEnvironment : private mir::DefaultServerConfiguration
{
public:
    MinimalServerEnvironment() : DefaultServerConfiguration(1, argv)
    {
    }

    ~MinimalServerEnvironment()
    {
        if (main_loop_thread.joinable())
        {
            the_main_loop()->stop();
            main_loop_thread.join();
        }
    }

    auto options() const -> std::shared_ptr<mir::options::Option>
    {
        return std::make_shared<mir::options::ProgramOption>();
    }

    auto console_services() -> std::shared_ptr<mir::ConsoleServices>
    {
        // The ConsoleServices may require a running mainloop.
        if (!main_loop_thread.joinable())
            main_loop_thread = std::thread{[this]() { the_main_loop()->run(); }};

        return the_console_services();
    }

    auto display_report() -> std::shared_ptr<mir::graphics::DisplayReport>
    {
        return the_display_report();
    }

    auto logger() -> std::shared_ptr<mir::logging::Logger>
    {
        return the_logger();
    }

    auto emergency_cleanup_registry() -> std::shared_ptr<mir::EmergencyCleanupRegistry>
    {
        return the_emergency_cleanup();
    }

    auto initial_display_configuration() -> std::shared_ptr<mir::graphics::DisplayConfigurationPolicy>
    {
        return the_display_configuration_policy();
    }

    auto gl_config() -> std::shared_ptr<mir::graphics::GLConfig>
    {
        return the_gl_config();
    }

private:
    std::thread main_loop_thread;
    static char const* argv[];
};
char const* MinimalServerEnvironment::argv[] = {"graphics_platform_test_harness", nullptr};

std::string describe_probe_result(mg::probe::Result priority)
{
    if (priority == mg::probe::unsupported)
    {
        return "UNSUPPORTED";
    }
    else if (priority == mg::probe::dummy)
    {
        return "DUMMY";
    }
    else if (priority < mg::probe::supported)
    {
        return std::string{"SUPPORTED - "} + std::to_string(mg::probe::supported - priority);
    }
    else if (priority == mg::probe::supported)
    {
        return "SUPPORTED";
    }
    else if (priority < mg::probe::best)
    {
        return std::string{"SUPPORTED + "} + std::to_string(priority - mg::probe::supported);
    }
    else if (priority == mg::probe::best)
    {
        return "BEST";
    }
    return std::string{"BEST + "} + std::to_string(priority - mg::probe::best);
}

auto test_probe(mir::SharedLibrary const& dso, MinimalServerEnvironment& config) -> std::vector<mg::SupportedDevice>
{
    try
    {
        auto probe_fn =
            dso.load_function<mg::PlatformProbe>("probe_graphics_platform", MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

        auto result = probe_fn(
            config.console_services(),
            std::make_shared<mir::udev::Context>(),
            *std::dynamic_pointer_cast<mir::options::ProgramOption>(config.options()));

        for (auto const& device : result)
        {
            std::cout << "Probe result: " << describe_probe_result(device.support_level) << "(" << device.support_level << ")" << std::endl;
        }

        return result;
    }
    catch (...)
    {
        std::throw_with_nested(std::runtime_error{"Failure in probe()"});
    }
}

auto test_platform_construction(mir::SharedLibrary const& dso, std::vector<mg::SupportedDevice> const& devices, MinimalServerEnvironment& env)
    -> std::shared_ptr<mir::graphics::DisplayPlatform>
{
    try
    {
        auto create_display_platform = dso.load_function<mg::CreateDisplayPlatform>(
            "create_display_platform", MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

        for (auto const& device : devices)
        {
            if (device.support_level >= mg::probe::supported)
            {
                auto display = create_display_platform(
                    device,
                    env.options(),
                    env.emergency_cleanup_registry(),
                    env.console_services(),
                    env.display_report());

                std::cout << "Successfully constructed DisplayPlatform" << std::endl;

                return display;
            }
        }
        BOOST_THROW_EXCEPTION((std::runtime_error{"Device probe didn't return any supported devices"}));

    }
    catch (...)
    {
        std::throw_with_nested(std::runtime_error{"Failure constructing platform"});
    }
}

auto test_display_construction(mir::graphics::DisplayPlatform& platform, MinimalServerEnvironment& env)
    -> std::shared_ptr<mir::graphics::Display>
{
    try
    {
        auto display = platform.create_display(
            env.initial_display_configuration(),
            env.gl_config());

        std::cout << "Successfully created display" << std::endl;

        return display;
    }
    catch (...)
    {
        std::throw_with_nested(std::runtime_error{"Failure constructing Display"});
    }
}

void for_each_display_buffer(mg::Display& display, std::function<void(mg::DisplaySink&)> functor)
{
    display.for_each_display_sync_group(
        [db_functor = std::move(functor)](mg::DisplaySyncGroup& sync_group)
        {
            sync_group.for_each_display_sink(db_functor);
        });
}

auto test_display_has_at_least_one_enabled_output(mg::Display& display) -> bool
{
    int output_count{0};
    for_each_display_buffer(display, [&output_count](auto&) { ++output_count; });
    if (output_count > 0)
    {
        std::cout << "Display has " << output_count << " enabled outputs" << std::endl;
    }
    else
    {
        std::cout << "Display has no enabled outputs!" << std::endl;
    }

    return output_count > 0;
}

template<typename Period, typename Rep>
auto bounce_within(
    mir::geometry::Rectangle const& surface,
    mir::geometry::Displacement const& motion_per_ms,
    std::chrono::duration<Rep, Period> const& duration,
    mir::geometry::Rectangle const& bounding_box)
    -> mir::geometry::Displacement
{
    auto bounced_motion = motion_per_ms;
    auto const projected_motion =
        motion_per_ms * std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    if ((surface.bottom_right() + projected_motion).x > bounding_box.right())
    {
        bounced_motion =
            mir::geometry::Displacement{
                bounced_motion.dx * -1,
                bounced_motion.dy};
    }
    if ((surface.bottom_right() + projected_motion).y > bounding_box.bottom())
    {
        bounced_motion =
            mir::geometry::Displacement{
                bounced_motion.dx,
                bounced_motion.dy * -1};
    }
    if ((surface.bottom_right() + projected_motion).x.as_int() < 0)
    {
        bounced_motion =
            mir::geometry::Displacement{
                bounced_motion.dx * -1,
                bounced_motion.dy};
    }
    if ((surface.bottom_right() + projected_motion).y.as_int() < 0)
    {
        bounced_motion =
            mir::geometry::Displacement{
                bounced_motion.dx,
                bounced_motion.dy * -1};
    }

    return bounced_motion;
}

void print_diagnostic_information(std::exception const& err)
{
    std::cout << "Error: " << err.what() << std::endl;
    std::cout << boost::diagnostic_information(err) << std::endl;
    try
    {
        std::rethrow_if_nested(err);
    }
    catch (std::exception const& inner)
    {
        std::cout << "Caused by: " << std::endl;
        print_diagnostic_information(inner);
    }
    catch (...)
    {
        std::cout << "Caused by broken exception!" << std::endl;
    }
}
}

int main(int argc, char const** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " PLATFORM_DSO" << std::endl;
        return -1;
    }

    mir::SharedLibrary platform_dso{argv[1]};

    MinimalServerEnvironment config;

    bool success = true;
    try
    {
        auto devices = test_probe(platform_dso, config);
        if (auto platform = test_platform_construction(platform_dso, devices, config))
        {
            if (auto display = test_display_construction(*platform, config))
            {
                success &= test_display_has_at_least_one_enabled_output(*display);
            }
            else
            {
                success = false;
            }
        }
        else
        {
            success = false;
        }
    }
    catch (std::exception const& e)
    {
        print_diagnostic_information(e);
        success = false;
    }
    catch (...)
    {
        std::cout << "Error: Broken exception" << std::endl;
        success = false;
    }
    std::cout << (success ? "PASS" : "FAILURE") << std::endl;
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
