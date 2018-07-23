/*
 * Copyright © 2012 Canonical Ltd.
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
 *   Thomas Guest  <thomas.guest@canonical.com>
 */

#ifndef MIR_GRAPHICS_PLATFORM_H_
#define MIR_GRAPHICS_PLATFORM_H_

#include <boost/program_options/options_description.hpp>

#include "mir/extension_description.h"
#include "mir/module_properties.h"
#include "mir/module_deleter.h"

namespace mir
{
class EmergencyCleanupRegistry;
class ConsoleServices;

namespace logging { class Logger; }

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
class PlatformIpcOperations;
class PlatformAuthentication;
class NativeRenderingPlatform
{
protected:
    NativeRenderingPlatform() = default;
    virtual ~NativeRenderingPlatform() = default;
    NativeRenderingPlatform(NativeRenderingPlatform const&) = delete;
    NativeRenderingPlatform& operator=(NativeRenderingPlatform const&) = delete;
};

/**
 * \defgroup platform_enablement Mir platform enablement
 *
 * Classes and functions that need to be implemented to add support for a graphics platform.
 */

/**
 * Interface to platform specific support for graphics operations.
 * \ingroup platform_enablement
 */
class RenderingPlatform
{
public:
    RenderingPlatform() = default;
    RenderingPlatform(RenderingPlatform const& p) = delete;
    RenderingPlatform& operator=(RenderingPlatform const& p) = delete;

    virtual ~RenderingPlatform() = default;
    /**
     * Creates the buffer allocator subsystem.
     */
    virtual UniqueModulePtr<GraphicBufferAllocator> create_buffer_allocator() = 0;

    /**
     * Creates an object capable of doing platform specific processing of buffers
     * before they are sent or after they are received across IPC
     */
    virtual UniqueModulePtr<PlatformIpcOperations> make_ipc_operations() const = 0;

    /**
     * Access the native resource[s] used to connect to the rendering backend
     * for this platform
     */
    virtual NativeRenderingPlatform* native_rendering_platform() = 0;
};

class NativeDisplayPlatform
{
protected:
    NativeDisplayPlatform() = default;
    virtual ~NativeDisplayPlatform() = default;
    NativeDisplayPlatform(NativeDisplayPlatform const&) = delete;
    NativeDisplayPlatform& operator=(NativeDisplayPlatform const&) = delete;
};

class DisplayPlatform
{
public:
    DisplayPlatform() = default;
    DisplayPlatform(DisplayPlatform const& p) = delete;
    DisplayPlatform& operator=(DisplayPlatform const& p) = delete;

    virtual ~DisplayPlatform() = default;

    /**
     * Creates the display subsystem.
     */
    virtual UniqueModulePtr<Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<GLConfig> const& gl_config) = 0;

    /**
     * Access the platform-specific resource[s] from the display.
     */
    virtual NativeDisplayPlatform* native_display_platform() = 0;

    /**
     * Get the extensions for the platform.
     */
    virtual std::vector<ExtensionDescription> extensions() const = 0;
};

class Platform : public DisplayPlatform,
                 public RenderingPlatform
{
public:
    Platform() = default;
    Platform(const Platform& p) = delete;
    Platform& operator=(const Platform& p) = delete;

    virtual ~Platform() = default;
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

//The host/guest platform concept is soon to be removed. Use CreateRenderingPlatform and
//CreateDisplayPlatform instead
typedef mir::UniqueModulePtr<mir::graphics::Platform>(*CreateHostPlatform)(
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mir::graphics::DisplayReport> const& report,
    std::shared_ptr<mir::logging::Logger> const& logger);

typedef void(*AddPlatformOptions)(
    boost::program_options::options_description& config);

typedef mir::graphics::PlatformPriority(*PlatformProbe)(
    std::shared_ptr<mir::ConsoleServices> const&,
        mir::options::ProgramOption const& options);

typedef mir::ModuleProperties const*(*DescribeModule)();

typedef mir::UniqueModulePtr<mir::graphics::DisplayPlatform>(*CreateDisplayPlatform)(
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mir::graphics::DisplayReport> const& report,
    std::shared_ptr<mir::logging::Logger> const& logger);

typedef mir::UniqueModulePtr<mir::graphics::RenderingPlatform>(*CreateRenderingPlatform)(
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<mir::graphics::PlatformAuthentication> const& platform_authentication);

/** Signatures from the obsolete 0.27 ABI */
namespace obsolete_0_27
{
constexpr char const* symbol_version = "MIR_GRAPHICS_PLATFORM_0.27";

typedef mir::UniqueModulePtr<mir::graphics::Platform>(*CreateHostPlatform)(
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mir::graphics::DisplayReport> const& report,
    std::shared_ptr<mir::logging::Logger> const& logger);

typedef mir::graphics::PlatformPriority(*PlatformProbe)(mir::options::ProgramOption const& options);

typedef mir::UniqueModulePtr<mir::graphics::DisplayPlatform>(*CreateDisplayPlatform)(
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mir::graphics::DisplayReport> const& report,
    std::shared_ptr<mir::logging::Logger> const& logger);
}
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
 * Function prototype used to return a new host graphics platform. The host graphics platform
 * is the system entity that owns the physical display and is a mir host server.
 *
 * \param [in] options options to use for this platform
 * \param [in] emergency_cleanup_registry object to register emergency shutdown handlers with
 * \param [in] console console-services provider
 * \param [in] report the object to use to report interesting events from the display subsystem
 * \param [in] logger the object to use to log interesting events from the display subsystem
 *
 * This factory function needs to be implemented by each platform.
 *
 * \ingroup platform_enablement
 */
mir::UniqueModulePtr<mir::graphics::Platform> create_host_platform(
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mir::graphics::DisplayReport> const& report,
    std::shared_ptr<mir::logging::Logger> const& logger);

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
mir::graphics::PlatformPriority probe_graphics_platform(
    std::shared_ptr<mir::ConsoleServices> const& console,
    mir::options::ProgramOption const& options);

mir::ModuleProperties const* describe_graphics_module();

mir::UniqueModulePtr<mir::graphics::DisplayPlatform> create_display_platform(
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mir::graphics::DisplayReport> const& report,
    std::shared_ptr<mir::logging::Logger> const& logger);

mir::UniqueModulePtr<mir::graphics::RenderingPlatform> create_rendering_platform(
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<mir::graphics::PlatformAuthentication> const& platform_authentication);

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
}

#endif // MIR_GRAPHICS_PLATFORM_H_
