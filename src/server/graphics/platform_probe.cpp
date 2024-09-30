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

 #include "platform_probe.h"

#include "mir/graphics/display.h"
#include "mir/log.h"
#include "mir/graphics/platform.h"
#include "mir/options/configuration.h"
#include "mir/shared_library.h"
#include "mir/shared_library_prober.h"
#include "mir/shared_library_prober_report.h"
#include "mir/udev/wrapper.h"

#include <algorithm>
#include <boost/throw_exception.hpp>
#include <dlfcn.h>

namespace mg = mir::graphics;
namespace mo = mir::options;

namespace
{
auto probe_module(
    std::function<std::vector<mg::SupportedDevice>()> const& probe,
    mir::SharedLibrary const& module,
    char const* platform_type_name) -> std::vector<mg::SupportedDevice>
{
    auto describe = module.load_function<mir::graphics::DescribeModule>(
        "describe_graphics_module",
        MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

    auto desc = describe();
    mir::log_info("Found %s driver: %s (version %d.%d.%d)",
                  platform_type_name,
                  desc->name,
                  desc->major_version,
                  desc->minor_version,
                  desc->micro_version);

    auto supported_devices = probe();
    if (supported_devices.empty())
    {
        mir::log_info("(Unsupported by system environment)");
    }
    else
    {
        mir::log_info("Driver supports:");
        for (auto const& device : supported_devices)
        {
            auto const device_name =
                [&device]() -> std::string
                {
                    if (device.device)
                    {
                        // If the platform claims a particular device, name it.
                        return device.device->devpath();
                    }
                    else
                    {
                        // The platform claims it can support the system in general
                        return "System";
                    }
                }();
            mir::log_info("\t%s (priority %i)", device_name.c_str(), device.support_level);
        }
    }
    return supported_devices;
}

auto is_graphics_module(mir::SharedLibrary const& module) -> bool
{
    try
    {
        module.load_function<mg::DescribeModule>("describe_graphics_module", MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
        return true;
    }
    catch (std::exception const&)
    {
        return false;
    }
}
}

auto mir::graphics::probe_display_module(
    SharedLibrary const& module,
    mo::Option const& options,
    std::shared_ptr<ConsoleServices> const& console) -> std::vector<SupportedDevice>
{
    return probe_module(
        [&console, &options, &module]() -> std::vector<mg::SupportedDevice>
        {
            auto probe = module.load_function<mir::graphics::PlatformProbe>(
                "probe_display_platform",
                MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
            return probe(console, std::make_shared<mir::udev::Context>(), options);
        },
        module,
        "display");
}

auto mir::graphics::probe_rendering_module(
    std::span<std::shared_ptr<mg::DisplayPlatform>> const& platforms,
    SharedLibrary const& module,
    mo::Option const& options,
    std::shared_ptr<ConsoleServices> const& console) -> std::vector<SupportedDevice>
{
    return probe_module(
        [&console, &options, &module, &platforms]() -> std::vector<SupportedDevice>
        {
            auto probe = module.load_function<mg::RenderProbe>(
                "probe_rendering_platform",
                MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
            return probe(platforms, *console, std::make_shared<mir::udev::Context>(), options);
        },
        module,
        "rendering");
}

namespace
{
bool is_same_device(
    std::unique_ptr<mir::udev::Device> const& a,
    std::unique_ptr<mir::udev::Device> const& b)
{
    /* TODO: This compares for exact udev path equality. We *may* want to treat
     * some devices as equal even if they have different device paths.
     */
    if (!a || !b)
    {
        // A null device is no equal to any other device, not even another null device
        return false;
    }
    return *a == *b;
}

}


// GCC and Clang both ensure the switch is exhaustive.
// GCC, however, gets a "control reaches end of non-void function" warning without this
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#endif

auto mg::modules_for_device(
    std::function<std::vector<mg::SupportedDevice>(mir::SharedLibrary const&)> const& probe,
    std::vector<std::shared_ptr<mir::SharedLibrary>> const& modules,
    mg::TypePreference nested_selection)
    -> std::vector<std::pair<mg::SupportedDevice, std::shared_ptr<mir::SharedLibrary>>>
{
    // We may have multiple hardware platforms, and need to load all of them
    std::vector<std::pair<mg::SupportedDevice, std::shared_ptr<mir::SharedLibrary>>> best_modules_so_far;
    // We will only ever want to load at most one nested platform
    std::optional<std::pair<mg::SupportedDevice, std::shared_ptr<mir::SharedLibrary>>> best_nested;
    for (auto& module : modules)
    {
        try
        {
            auto supported_devices = probe(*module);
            for (auto& device : supported_devices)
            {
                if (device.device)
                {
                    auto const existing_device = std::find_if(
                        best_modules_so_far.begin(),
                        best_modules_so_far.end(),
                        [&device](auto& candidate)
                        {
                            return is_same_device(device.device, candidate.first.device);
                        });
                    if (existing_device != best_modules_so_far.end())
                    {
                        // This device is also supported by some other platform; pick only the best one
                        if (existing_device->first.support_level < device.support_level)
                        {
                            *existing_device = std::make_pair(std::move(device), module);
                        }
                    }
                    else if (device.support_level > mg::probe::unsupported)
                    {
                        // Not-seen-before device, which this platform supports in some fashion
                        best_modules_so_far.emplace_back(std::move(device), module);
                    }
                }
                else
                {
                    /* Platforms with no associated `mir::udev::Device` are *nested* platforms
                     * We want at most one of these
                     */

                    // This could be more elegant with std::optional<>::transform(), but not C++23 for me!
                    auto const current_best_support = [&best_nested]() {
                        if (best_nested)
                        {
                            return best_nested->first.support_level;
                        }
                        return mg::probe::unsupported;
                    }();
                    if (device.support_level > current_best_support)
                    {
                        best_nested = std::make_pair(std::move(device), module);
                    }
                }
            }
        }
        catch (std::runtime_error const&)
        {
        }
    }
    if (!best_modules_so_far.empty())
    {
        // If there are any non-dummy platforms in our support list, remove all the dummy platforms.

        // First, move all the non-dummy platforms to the front…
        auto first_dummy_platform = std::partition(
            best_modules_so_far.begin(),
            best_modules_so_far.end(),
            [](auto const& module)
            {
                return module.first.support_level > mg::probe::dummy;
            });

        // …then, if there are any platforms before the start of the dummy platforms…
        if (first_dummy_platform != best_modules_so_far.begin())
        {
            // …erase everything after the start of the dummy platforms.
            best_modules_so_far.erase(first_dummy_platform, best_modules_so_far.end());
        }
    }

    if (best_modules_so_far.empty())
    {
        if (!best_nested)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to find any platforms for current system"}));
        }
        // Our *only* option is the nested platform; return that.
        decltype(best_modules_so_far) nested_vec;
        nested_vec.push_back(std::move(best_nested.value()));
        return nested_vec;
    }

    switch(nested_selection)
    {
    case TypePreference::prefer_nested:
        if (best_nested && best_nested->first.support_level >= mg::probe::supported)
        {
            decltype(best_modules_so_far) nested_vec;
            nested_vec.push_back(std::move(best_nested.value()));
            return nested_vec;
        }
        return best_modules_so_far;
    case TypePreference::prefer_hardware:
        if (!best_modules_so_far.empty())
        {
            return best_modules_so_far;
        }
        decltype(best_modules_so_far) nested_vec;
        nested_vec.push_back(std::move(best_nested.value()));
        return nested_vec;
    }
}

#ifndef __clang__
#pragma GCC diagnostic push
#endif

auto mir::graphics::display_modules_for_device(
    std::vector<std::shared_ptr<SharedLibrary>> const& modules,
    mo::Configuration const& options,
    std::shared_ptr<ConsoleServices> const& console) -> std::vector<std::pair<SupportedDevice, std::shared_ptr<SharedLibrary>>>
{
    return modules_for_device(
        [&options, &console](mir::SharedLibrary const& module) -> std::vector<mg::SupportedDevice>
        {
            return mg::probe_display_module(module, *options.the_options_for(module), console);
        },
        modules,
        TypePreference::prefer_nested);
}

auto mir::graphics::rendering_modules_for_device(
    std::vector<std::shared_ptr<SharedLibrary>> const& modules,
    std::span<std::shared_ptr<DisplayPlatform>> const& platforms,
    mo::Configuration const& options,
    std::shared_ptr<ConsoleServices> const& console) -> std::vector<std::pair<SupportedDevice, std::shared_ptr<SharedLibrary>>>
{
    return modules_for_device(
        [&platforms, &options, &console](SharedLibrary const& module) -> std::vector<SupportedDevice>
        {
            if (is_graphics_module(module))
            {
                return probe_rendering_module(platforms, module, *options.the_options_for(module), console);
            }
            else
            {
                return {};
            }
        },
        modules,
        TypePreference::prefer_hardware);
}

namespace
{
auto split_on(std::string const& tokens, char delimiter) -> std::vector<std::string>
{
    std::string token;
    std::istringstream token_stream{tokens};

    std::vector<std::string> result;
    while (std::getline(token_stream, token, delimiter))
    {
        result.push_back(token);
    }

    return result;
}

// Precondition: `modules` contains only graphics platform modules (ie: DSOs with a `describe_graphics_module` symbol)
auto select_platforms_from_list(std::string const& selection, std::vector<std::shared_ptr<mir::SharedLibrary>> const& modules)
    -> std::vector<std::shared_ptr<mir::SharedLibrary>>
{
    std::vector<std::shared_ptr<mir::SharedLibrary>> selected_modules;
    std::vector<std::string> found_module_names;

    // Our platform modules are a comma-delimited list.
    auto requested_modules = split_on(selection, ',');

    for (auto const& module : modules)
    {
        auto describe_module = module->load_function<mg::DescribeModule>(
            "describe_graphics_module",
            MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
        auto const description = describe_module();
        found_module_names.emplace_back(description->name);

        if (auto const i = std::find(requested_modules.begin(), requested_modules.end(), description->name);
            i != requested_modules.end())
        {
            selected_modules.push_back(module);
            requested_modules.erase(i);
        }
    }

    if (!requested_modules.empty())
    {
        std::stringstream error_msg;
        error_msg << "Failed to find all requested platform modules." << std::endl;
        error_msg << "Detected modules are: " << std::endl;
        for (auto const& module : found_module_names)
        {
            error_msg << "\t" << module << std::endl;
        }
        error_msg << "Failed to find:" << std::endl;
        for (auto const& module : requested_modules)
        {
            error_msg << "\t" << module << std::endl;
        }
        BOOST_THROW_EXCEPTION((std::runtime_error{error_msg.str()}));
    }

    return selected_modules;
}

auto dso_filename_alphabetically_before(mir::SharedLibrary const& a, mir::SharedLibrary const& b) -> bool
{
    Dl_info info_a, info_b;

    auto describe_a = a.load_function<mir::graphics::DescribeModule>("describe_graphics_module", MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
    auto describe_b = b.load_function<mir::graphics::DescribeModule>("describe_graphics_module", MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

    dladdr(reinterpret_cast<void const*>(describe_a), &info_a);
    dladdr(reinterpret_cast<void const*>(describe_b), &info_b);
    return strcmp(info_a.dli_fname, info_b.dli_fname) < 0;
}
}

auto mg::select_display_modules(
    options::Configuration const& options,
    std::shared_ptr<ConsoleServices> const& console,
    SharedLibraryProberReport& lib_loader_report)
    -> std::vector<std::pair<SupportedDevice, std::shared_ptr<SharedLibrary>>>
{
    std::vector<std::pair<SupportedDevice, std::shared_ptr<SharedLibrary>>> platform_modules;

    auto const global_options = options.global_options();
    auto const& path = global_options->get<std::string>(options::platform_path);
    auto platforms = mir::libraries_for_path(path, lib_loader_report);

    if (platforms.empty())
    {
        auto msg = "Failed to find any platform plugins in: " + path;
        BOOST_THROW_EXCEPTION((std::runtime_error{msg.c_str()}));
    }

    /* Remove any non-graphics modules from the list.
     * This lets us assume, from here on in, that everything in platforms is
     * a graphics module (and hence contains a `describe_graphics_module` symbol, etc)
     */
    platforms.erase(
        std::remove_if(
            platforms.begin(),
            platforms.end(),
            [](auto const& module)
            {
                return !is_graphics_module(*module);
            }),
        platforms.end());

    /* Sort the platform modules by DSO filename.
     *
     * (One part of) The selection algorithm stores the first platform that
     * claims to support a given device, and then replaces that platform if
     * a better one comes along. The upshot of this is that if two platforms
     * claim the same level of support, we'll pick the first one seen.
     *
     * The order of the platforms we have here is just whatever order the filesystem
     * returned when we enumerated the directory; that's not guaranteed to be any
     * particular order (and can be filesystem specific).
     *
     * To make platform selection deterministic, first sort the platforms by
     * the only thing that is guaranteed to be unique: their filenames.
     *
     * Since we want to prefer the X11 platform over the nested Wayland platform when
     * both are available, we sort in *reverse* alphabetical order, so “x11” comes before
     * “wayland”
     */
    std::sort(
        platforms.begin(), platforms.end(),
        [](auto const& lhs, auto const& rhs)
        {
            return !dso_filename_alphabetically_before(*lhs, *rhs);
        });

    /* Treat the Virtual platform specially:
     *
     * It does not participate in the regular platform selection (it does not count as either a
     * hardware nor a nested platform).
     *
     * Instead, the normal probing for platforms should be done, and then the virtual platform loaded
     * if and only if the user has passed a Virtual platform option.
     */
    auto virtual_platform_pos = std::find_if(
        platforms.begin(),
        platforms.end(),
        [](auto const& module)
        {
            auto describe = module->template load_function<mir::graphics::DescribeModule>(
                "describe_graphics_module",
                MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
            return strcmp("mir:virtual", describe()->name) == 0;
        });
    auto virtual_platform = virtual_platform_pos != platforms.end() ? *virtual_platform_pos : std::shared_ptr<SharedLibrary>{};

    if (global_options->is_set(options::platform_display_libs))
    {
        auto const manually_selected_platforms =
            select_platforms_from_list(global_options->get<std::string>(options::platform_display_libs), platforms);

        for (auto const& platform : manually_selected_platforms)
        {
            auto supported_devices =
                graphics::probe_display_module(
                    *platform,
                    *options.the_options_for(*platform),
                    console);

            bool found_supported_device{false};
            for (auto& device : supported_devices)
            {
                // Add any devices that the platform claims are supported
                if (device.support_level >= mg::probe::supported)
                {
                    found_supported_device = true;
                    /* We want to check that, if the virtual platform is manually specified, that
                     * it's actually going to be used and warn otherwise, but we're going to
                     * explicitly probe it later regardless, so we don't want to add it twice.
                     */
                    if (platform != virtual_platform)
                    {
                        platform_modules.emplace_back(std::move(device), platform);
                    }
                }
            }

            if (!found_supported_device)
            {
                auto const describe_module = platform->load_function<mg::DescribeModule>(
                    "describe_graphics_module",
                    MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
                auto const descriptor = describe_module();

                mir::log_warning("Manually-specified display platform %s does not claim to support this system. Trying anyway...", descriptor->name);

                // We're here only if the platform doesn't claim to support *any* of the detected devices
                // Add *all* the found devices into our platform list, and hope.
                for (auto& device : supported_devices)
                {
                    platform_modules.emplace_back(std::move(device), platform);
                }
            }
        }
    }
    else
    {
        if (virtual_platform)
        {
            // We don't need to probe the virtual platform; that is done separately below.
            platforms.erase(virtual_platform_pos);
        }
        platform_modules = display_modules_for_device(platforms, options, console);
    }

    if (virtual_platform)
    {
        auto virtual_probe = probe_display_module(
            *virtual_platform, *options.the_options_for(*virtual_platform), console);
        if (virtual_probe.size() && virtual_probe.front().support_level >= mg::probe::supported)
        {
            platform_modules.emplace_back(std::move(virtual_probe.front()), std::move(virtual_platform));
        }
    }
    return platform_modules;
}

auto mg::select_buffer_allocating_renderer(
    Display& display,
    std::span<std::shared_ptr<RenderingPlatform>> rendering_platforms) -> std::shared_ptr<RenderingPlatform>
{
    /* Theory:
     * We iterate over each DisplaySink and each RenderingPlatform, and
     * collate:
     * *) If the RenderingPlatform is *best* at driving that sink, and
     * *) If the RenderingPlatform is *unable* to drive that sink
     *
     * We then filter to just the RenderingPlatform(s) that are able to
     * drive the most DisplaySink.
     *
     * Of that list, we pick the RenderingPlatform that has the highest
     * number of “best” suitabilities.
     */
    struct PlatformData
    {
        std::shared_ptr<mg::RenderingPlatform> platform;
        std::shared_ptr<mg::GLRenderingProvider> provider;
        int best_count;
        int unsupported_count;
    };

    std::vector<PlatformData> providers;
    providers.reserve(rendering_platforms.size());

    for (auto const& platform : rendering_platforms)
    {
        providers.push_back({
                platform,
                platform->acquire_provider<mg::GLRenderingProvider>(platform),
                0,
                0
        });
    }

    display.for_each_display_sync_group(
        [&](auto& sync_group)
        {
            sync_group.for_each_display_sink(
                [&](auto& sink)
                {
                    for (auto &[platform, provider, best_count, unsupported_count] : providers)
                    {
                        auto suitability = provider->suitability_for_display(sink);
                        if (suitability >= mg::probe::best)
                        {
                            best_count++;
                        }
                        else if (suitability == mg::probe::unsupported)
                        {
                            unsupported_count++;
                        }
                    }
                });
        });

    // providers.size() is guaranteed to be ≥ 1, as we're guaranteed to have at least one rendering platform

    auto const least_unsupported_count = std::min_element(
        providers.begin(), providers.end(),
        [](auto const& a, auto const& b) { return a.unsupported_count < b.unsupported_count; })->unsupported_count;

    auto const last_good_provider = std::remove_if(
        providers.begin(), providers.end(),
        [least_unsupported_count](auto const& provider) { return provider.unsupported_count > least_unsupported_count; });

    // Now, find the best platform out of the ones that support the most Sinks
    auto const best_provider = std::max_element(
        providers.begin(), last_good_provider,
        [](auto const& a, auto const&b) { return a.best_count < b.best_count; });

    return best_provider->platform;
}
