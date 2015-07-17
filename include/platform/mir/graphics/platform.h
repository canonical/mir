/*
 * Copyright © 2012 Canonical Ltd.
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
 *   Thomas Guest  <thomas.guest@canonical.com>
 */

#ifndef MIR_GRAPHICS_PLATFORM_H_
#define MIR_GRAPHICS_PLATFORM_H_

#include <boost/program_options/options_description.hpp>
#include <EGL/egl.h>
#include <memory>

#include "mir/module_properties.h"

namespace mir
{
class EmergencyCleanupRegistry;

namespace frontend
{
class Surface;
}
namespace options
{
class Option;
class ProgramOption;
}

/// Graphics subsystem. Mediates interaction between core system and
/// the graphics environment.
namespace graphics
{
class Buffer;
class Display;
class DisplayReport;
class DisplayConfigurationPolicy;
class GraphicBufferAllocator;
class GLConfig;
class GLProgramFactory;
class PlatformIpcOperations;
class NestedContext;

/**
 * \defgroup platform_enablement Mir platform enablement
 *
 * Classes and functions that need to be implemented to add support for a graphics platform.
 */

/**
 * Interface to platform specific support for graphics operations.
 * \ingroup platform_enablement
 */
class Platform
{
public:
    Platform() = default;
    Platform(const Platform& p) = delete;
    Platform& operator=(const Platform& p) = delete;

    virtual ~Platform() { /* TODO: make nothrow */ }

    /**
     * Creates the buffer allocator subsystem.
     */
    virtual std::shared_ptr<GraphicBufferAllocator> create_buffer_allocator() = 0;
    
    /**
     * Creates the display subsystem.
     */
    virtual std::shared_ptr<Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<GLProgramFactory> const& gl_program_factory,
        std::shared_ptr<GLConfig> const& gl_config) = 0;

    /**
     * Creates an object capable of doing platform specific processing of buffers
     * before they are sent or after they are recieved accross IPC
     */
    virtual std::shared_ptr<PlatformIpcOperations> make_ipc_operations() const = 0;

    virtual EGLNativeDisplayType egl_native_display() const = 0;
};

/**
 * A measure of how well a platform supports a device
 *
 * \note This is compared as an integer; best + 1 is a valid PlatformPriority that
 *       will be used in preference to a module that reports best.
 *       Platform modules distributed with Mir will never use a priority higher
 *       than best.
 */
enum PlatformPriority : uint32_t
{
    unsupported = 0,    /**< Unable to function at all on this device */
    dummy = 1,          /**< Used only for dummy or stub platforms.
                         */
    supported = 128,    /**< Capable of providing a functioning Platform on this device,
                         *    possibly with degraded performance or features.
                         */
    best = 256          /**< Capable of providing a Platform with the best features and
                         *   performance this device is capable of.
                         */
};

typedef std::shared_ptr<mir::graphics::Platform>(*CreateHostPlatform)(
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mir::graphics::DisplayReport> const& report);

typedef std::shared_ptr<mir::graphics::Platform>(*CreateGuestPlatform)(
    std::shared_ptr<mir::graphics::DisplayReport> const& report,
    std::shared_ptr<mir::graphics::NestedContext> const& nested_context);


typedef void(*AddPlatformOptions)(
    boost::program_options::options_description& config);

typedef mir::graphics::PlatformPriority(*PlatformProbe)(mir::options::ProgramOption const& options);

typedef mir::ModuleProperties const*(*DescribeModule)();
}
}

extern "C"
{

/**
 * Function prototype used to return a new host graphics platform. The host graphics platform
 * is the system entity that owns the physical display and is a mir host server.
 *
 * \param [in] options options to use for this platform
 * \param [in] emergency_cleanup_registry object to register emergency shutdown handlers with
 * \param [in] report the object to use to report interesting events from the display subsystem
 *
 * This factory function needs to be implemented by each platform.
 *
 * \ingroup platform_enablement
 */
std::shared_ptr<mir::graphics::Platform> create_host_platform(
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mir::graphics::DisplayReport> const& report);

/**
 * Function prototype used to return a new guest graphics platform. The guest graphics platform
 * exists alongside the host platform and do not output or control the physical displays
 *
 * \param [in] nested_context the object that contains resources needed from the host platform
 * \param [in] report the object to use to report interesting events from the display subsystem
 *
 * This factory function needs to be implemented by each platform.
 *
 * \ingroup platform_enablement
 */
std::shared_ptr<mir::graphics::Platform> create_guest_platform(
    std::shared_ptr<mir::graphics::DisplayReport> const& report,
    std::shared_ptr<mir::graphics::NestedContext> const& nested_context);

/**
 * Function prototype used to add platform specific options to the platform-independent server options.
 *
 * \param [in] config a boost::program_options that can be appended with new options
 *
 * This factory function needs to be implemented by each platform.
 *
 * \ingroup platform_enablement
 */
void add_graphics_platform_options(
    boost::program_options::options_description& config);

// TODO: We actually need to be more granular here; on a device with more
//       than one graphics system we may need a different platform per GPU,
//       so we should be associating platforms with graphics devices in some way
mir::graphics::PlatformPriority probe_graphics_platform(mir::options::ProgramOption const& options);

mir::ModuleProperties const* describe_graphics_module();
}

#endif // MIR_GRAPHICS_PLATFORM_H_
