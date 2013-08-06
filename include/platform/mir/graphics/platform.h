/*
 * Copyright © 2012 Canonical Ltd.
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
 *
 * Authored by:
 *   Thomas Guest  <thomas.guest@canonical.com>
 */

#ifndef MIR_GRAPHICS_PLATFORM_H_
#define MIR_GRAPHICS_PLATFORM_H_

#include <memory>

namespace mir
{
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
class BufferIPCPacker;
class Buffer;
class Display;
struct PlatformIPCPackage;
class BufferInitializer;
class InternalClient;
class DisplayReport;
class DisplayConfigurationPolicy;
class GraphicBufferAllocator;

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
     *
     * \param [in] buffer_initializer the object responsible for initializing the buffers
     */

    virtual std::shared_ptr<GraphicBufferAllocator> create_buffer_allocator(
        std::shared_ptr<BufferInitializer> const& buffer_initializer) = 0;

    /**
     * Creates the display subsystem.
     */
    virtual std::shared_ptr<Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy) = 0;

    /**
     * Gets the IPC package for the platform.
     *
     * The IPC package will be sent to clients when they connect.
     */
    virtual std::shared_ptr<PlatformIPCPackage> get_ipc_package() = 0;

    /**
     * Fills the IPC package for a buffer.
     *
     * The Buffer IPC package will be sent to clients when receiving a buffer.
     * The implementation must use the provided packer object to perform the packing.
     *
     * \param [in] packer the object providing the packing functionality
     * \param [in] buffer the buffer to fill the IPC package for
     */
    virtual void fill_ipc_package(std::shared_ptr<BufferIPCPacker> const& packer,
                                  std::shared_ptr<graphics::Buffer> const& buffer) const = 0;

    /**
     * Creates the in-process client support object.
     */
    virtual std::shared_ptr<InternalClient> create_internal_client() = 0;
};

/**
 * Function prototype used to return a new graphics platform.
 *
 * \param [in] options options to use for this platform
 * \param [in] report the object to use to report interesting events from the display subsystem
 *
 * This factory function needs to be implemented by each platform.
 *
 * \ingroup platform_enablement
 */
extern "C" typedef std::shared_ptr<Platform>(*CreatePlatform)(std::shared_ptr<options::Option> const& options, std::shared_ptr<DisplayReport> const& report);
extern "C" std::shared_ptr<Platform> create_platform (std::shared_ptr<options::Option> const& options, std::shared_ptr<DisplayReport> const& report);
}
}

#endif // MIR_GRAPHICS_PLATFORM_H_
