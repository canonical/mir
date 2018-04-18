/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_PLATFORM_H_
#define MIR_INPUT_PLATFORM_H_

#include "mir/module_properties.h"

#include <mir/options/option.h>
#include <mir/module_deleter.h>

#include <boost/program_options/options_description.hpp>

#include <memory>

namespace mir
{
class EmergencyCleanupRegistry;
class ConsoleServices;

namespace dispatch
{
class Dispatchable;
}

namespace input
{
class InputDevice;
class InputReport;
class InputDeviceRegistry;
class InputPlatformPolicy;

enum class PlatformPriority : uint32_t
{
    unsupported = 0,
    dummy = 1,
    supported = 128,
    best = 256,
};

/**
 * Input Platform is used to discover and access available input devices.
 *
 * A platform implementation is supposed to handle device occurance events by
 * opening new device and registering them at the server's InputDeviceRegistry.
 * Likewise the InputDeviceRegistry shall be informed about removed input devices.
 *
 * The actual processing of user input is controlled through the mir::input::InputDevice interface.
 */
class Platform
{
public:
    Platform() = default;
    virtual ~Platform() = default;

    /*!
     * The dispatchable of the platform shall be used to monitor for devices.
     */
    virtual std::shared_ptr<mir::dispatch::Dispatchable> dispatchable() = 0;

    /*!
     * Request the platform to start monitoring for devices.
     */
    virtual void start() = 0;
    /*!
     * Request the platform to stop monitoring for devices.
     */
    virtual void stop() = 0;
    /*!
     * Request the platform to pause for device reconfiguration.
     */
    virtual void pause_for_config() = 0;
    /*!
     * Tell the platform to continue after device reconfiguration.
     */
    virtual void continue_after_config() = 0;

private:
    Platform(Platform const&) = delete;
    Platform& operator=(Platform const&) = delete;
};

typedef mir::UniqueModulePtr<Platform>(*CreatePlatform)(
    options::Option const& options,
    std::shared_ptr<EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<InputDeviceRegistry> const& input_device_registry,
    std::shared_ptr<ConsoleServices> const& console,
    std::shared_ptr<InputReport> const& report);

typedef void(*AddPlatformOptions)(
    boost::program_options::options_description& config);

typedef PlatformPriority(*ProbePlatform)(
    options::Option const& options);

typedef ModuleProperties const*(*DescribeModule)();

}
}

extern "C"
{
#if defined(__clang__)
#pragma clang diagnostic push
// These functions are given "C" linkage to avoid name-mangling, not for C compatibility.
// (We don't want a warning for doing this intentionally.)
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

/**
 * Function used to initialize an input platform.
 *
 * \param [in] options options to use for this platform
 * \param [in] emergency_cleanup_registry object to register emergency shutdown handlers with
 * \param [in] input_device_registry object to register input devices handled by this platform
 * \param [in] report the object to use to report interesting events from the input subsystem
 *
 * This factory function needs to be implemented by each platform.
 *
 * \ingroup platform_enablement
 */
mir::UniqueModulePtr<mir::input::Platform> create_input_platform(
    mir::options::Option const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mir::input::InputDeviceRegistry> const& input_device_registry,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mir::input::InputReport> const& report);

/**
 * Function used to add additional configuration options
 *
 * \param [in] config program option description that the platform may extend
 *
 * This function needs to be implemented by each platform.
 *
 * \ingroup platform_enablement
 */
void add_input_platform_options(boost::program_options::options_description& config);

/**
 * probe_platform should indicate whether the platform is able to work within
 * the current environment.
 *
 * \param [in] options program options of the mir server
 *
 * This function needs to be implemented by each platform.
 *
 * \ingroup platform_enablement
 */
mir::input::PlatformPriority probe_input_platform(mir::options::Option const& options);

/**
 * describe_input_module should return a description of the input platform.
 *
 * This function needs to be implemented by each platform.
 *
 * \ingroup platform_enablement
 */
mir::ModuleProperties const* describe_input_module();

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
}
#endif // MIR_INPUT_PLATFORM_H_
