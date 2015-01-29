/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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

#include <mir/options/option.h>

#include <boost/program_options/options_description.hpp>

#include <memory>

namespace mir
{
class EmergencyCleanupRegistry;

namespace input
{
class InputEventHandlerRegister;
class InputDevice;
class InputReport;
class InputDeviceRegistry;

enum class PlatformPriority : uint32_t
{
    unsupported = 0,
    supported = 128,
    best = 256,
};

/**
 * Input Platform is used to discover and access available input devices.
 *
 * A platform implementation is supposed to handle device occurance events by
 * opening new device and register them at the server's InputDeviceRegistry.
 * Likewise the InputDeviceRegistry shall be informed about removed input devices.
 *
 * The actual processing of events is controlled through the mir::input::InputDevice interface.
 */
class Platform
{
public:
    Platform() = default;
    virtual ~Platform() = default;

    /*!
     * Request the platform to start monitoring for devices.
     *
     * \param input_device_registry should be informed about available input devices
     * \param trigger_registry should be used to register event sources that may indicate a changes of the available devices
     */
    virtual void start(InputEventHandlerRegister& trigger_registry, std::shared_ptr<InputDeviceRegistry> const& input_device_registry) = 0;
    /*!
     * Request the platform to stop monitoring for devices.
     */
    virtual void stop(InputEventHandlerRegister& trigger_registry) = 0;

private:
    Platform(Platform const&) = delete;
    Platform& operator=(Platform const&) = delete;
};

extern "C" typedef std::unique_ptr<Platform>(*CreatePlatform)(
    std::shared_ptr<options::Option> const& options,
    std::shared_ptr<EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<InputReport> const& report);
/**
 * Function used to initialize an input platform.
 *
 * \param [in] options options to use for this platform
 * \param [in] emergency_cleanup_registry object to register emergency shutdown handlers with
 * \param [in] report the object to use to report interesting events from the input subsystem
 *
 * This factory function needs to be implemented by each platform.
 *
 * \ingroup platform_enablement
 */
extern "C" std::unique_ptr<Platform> create_input_platform(
    std::shared_ptr<options::Option> const& options,
    std::shared_ptr<EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<InputReport> const& report);
extern "C" typedef void(*AddPlatformOptions)(
    boost::program_options::options_description& config);
/**
 * Function used to add additional configuration options
 *
 * \param [in] config program option description that the platform may extend
 *
 * This function needs to be implemented by each platform.
 *
 * \ingroup platform_enablement
 */
extern "C" void add_input_platform_options(
    boost::program_options::options_description& config);
extern "C" typedef PlatformPriority(*ProbePlatform)(
    options::Option const& options);
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
extern "C" PlatformPriority probe_input_platform(
    options::Option const& options);
}
}

#endif // MIR_INPUT_PLATFORM_H_
