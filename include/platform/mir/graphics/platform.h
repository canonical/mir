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
}

/// Graphics subsystem. Mediates interaction between core system and
/// the graphics environment.
namespace graphics
{
class Buffer;
class Display;
class InternalClient;
class DisplayReport;
class DisplayConfigurationPolicy;
class GraphicBufferAllocator;
class GLConfig;
class GLProgramFactory;
class PlatformIpcOperations;
class BufferWriter;

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
    
    virtual std::shared_ptr<BufferWriter> make_buffer_writer() = 0;

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
 * Function prototype used to return a new graphics platform.
 *
 * \param [in] options options to use for this platform
 * \param [in] emergency_cleanup_registry object to register emergency shutdown handlers with
 * \param [in] report the object to use to report interesting events from the display subsystem
 *
 * This factory function needs to be implemented by each platform.
 *
 * \ingroup platform_enablement
 */
extern "C" typedef std::shared_ptr<Platform>(*CreatePlatform)(
    std::shared_ptr<options::Option> const& options,
    std::shared_ptr<EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<DisplayReport> const& report);
extern "C" std::shared_ptr<Platform> create_platform(
    std::shared_ptr<options::Option> const& options,
    std::shared_ptr<EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<DisplayReport> const& report);
extern "C" typedef void(*AddPlatformOptions)(
    boost::program_options::options_description& config);
extern "C" void add_platform_options(
    boost::program_options::options_description& config);
}
}

#endif // MIR_GRAPHICS_PLATFORM_H_
