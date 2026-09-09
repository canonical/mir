/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_PLATFORM_GRAPHICS_RENDERING_PROVIDERS_H_
#define MIR_PLATFORM_GRAPHICS_RENDERING_PROVIDERS_H_

#include <mir/graphics/platform.h>
#include <mir/graphics/display_providers.h>
#include <mir/fd.h>
#include <memory>
#include <cstdint>

namespace mir::graphics
{

namespace gl
{
class Texture;
class OutputSurface;
}

/**
 * Provides integration with the OpenGL rendering API
 */
class GLRenderingProvider : public RenderingProvider
{
public:
    class Tag : public RenderingProvider::Tag
    {
    };

    /**
     * Get a GL texture as a view onto the Mir Buffer
     *
     * \param [in] buffer
     * \returns  A gl::Texture with the content of *buffer*.
     *           The returned texture may share ownership of *buffer*; it is not necessary
     *           for calling code to maintain a reference to *buffer*.
     *           If *buffer* is modified while the returned gl::Texture is live, results
     *           are undefined
     */
    virtual auto as_texture(std::shared_ptr<Buffer> buffer) -> std::shared_ptr<gl::Texture> = 0;

    /**
     * Create a rendering surface that can be output on DisplaySink
     *
     * \param [in] sink    The DisplaySink associated with this surface
     * \param [in] config  The config values that will be used for the EGL context
     */
    virtual auto surface_for_sink(DisplaySink& sink, GLConfig const& config) -> std::unique_ptr<gl::OutputSurface> = 0;
};

namespace drm { class Syncobj; }

/**
 * Provides integration with DRM sync objects
 *
 * A DRMRenderingProvider is a GLRendering provider that can additionally import
 * client-provided DRM syncobj fds for explicit buffer synchronisation.
 */
class DRMRenderingProvider : public GLRenderingProvider
{
public:
    class Tag : public RenderingProvider::Tag
    {
    };

    /**
     * Import a DRM syncobj from a file descriptor
     *
     * \param [in] syncobj_fd
     *     The syncobj fd to import. The caller does not need to keep the
     *     imported fd opened; the Mir object holds a reference to the necessary
     *     kernel resources.
     * \returns
     *     The imported drm::Syncobj.
     * \throws
     *     std::system_error on any underlying failure.
     */
    virtual auto import_syncobj(mir::Fd const& syncobj_fd) -> std::unique_ptr<drm::Syncobj> = 0;
};

class BlitterRenderingProvider : public RenderingProvider
{
public:
    class Tag : public RenderingProvider::Tag
    {
    };

    /**
     * Abstract handle for a group of rendering operations.
     *
     * Some hardware APIs have an asynchronous rendering mode,
     * where a group of operations can be submitted and processed
     * in order which then need to be waited on for completion.
     *
     * This is an abstract handle for any bookkeeping needed for such
     * an API.
     *
     * If uses of this rendering API need to be interleaved with
     * other rendering to a Surface then a Task must be created
     * after the other rendering is complete and then this Task
     * must be consumed by `wait_complete` before any further
     * access to the Surface.
     */
    class Task;

    /**
     * Handle to a target surface for the blitter
     */
    class Surface;

    /**
     * Set up any required bookkeeping for rendering.
     *
     * The returned Task holds the provided Surface.
     */
    auto create_task(std::unique_ptr<Surface> surf) -> std::unique_ptr<Task>;

    /**
     * Consume a Task and wait for all operations associated with it to complete.
     *
     * The returned Surface is the same one that was provided to `create_task`.
     */
    auto wait_complete(std::unique_ptr<Task> task) -> std::unique_ptr<Surface>;

    /**
     * Blit a source buffer to a target surface.
     *
     * \param task          The Task that holds the target Surface
     * \param source        A mir::Buffer the blit should read from.
     *                        Whether a specific buffer can be used as a source
     *                        depends on allocation-specific information.
     *                        If the blitter cannot use the provided buffer as
     *                        a source, this function will return false.
     * \param source_rect  The rectangle of the source buffer to read from.
     * \param target_rect  The rectangle of the target surface to write to.
     * \param rotation      The rotation to apply to the source buffer before writing to the target
     * \param mirror_mode The mirroring to apply to the source buffer before writing to the target
     * \return               true if the operation was successfully submitted.
     *                        If the operation was not submitted for any reason
     *                        (either the source buffer is not usable or some combination
     *                        of parameters are unsupported) then this function
     *                        will return `false` and will have had no effect on
     *                        the target surface or any previous operations.
     *                        `wait_complete` should be called on `task` to flush
     *                        any previous rendering and perform direct fallback rendering.
     */
    auto blit(
        Task& task,
        Buffer const& source,
        geometry::Rectangle const& source_rect,
        geometry::Rectangle const& target_rect,
        MirOrientation rotation,
        MirMirrorMode mirror_mode) -> bool;

    /**
     * Fill a rectangle of a target surface with a solid color, possibly alpha-blended.
     *
     * \param task          The Task that holds the target Surface
     * \param target_rect  The rectangle of the target surface to fill.
     * \param r             The red component of the fill color (0-255)
     * \param g             The green component of the fill color (0-255)
     * \param b             The blue component of the fill color (0-255)
     * \param a             The alpha component of the fill color (0-255)
     * \return               true if the operation was successfully submitted.
     *                        If the operation was not submitted for any reason
     *                        (either the source buffer is not usable or some combination
     *                        of parameters are unsupported) then this function
     *                        will return `false` and will have had no effect on
     *                        the target surface or any previous operations..
     *                        `wait_complete` should be called on `task` to flush
     *                        any previous rendering and perform direct fallback rendering.
     */
    auto fill(Task& task, geometry::Rectangle const& target_rect, uint8_t r, uint8_t g, uint8_t b, uint8_t a) -> bool;

    /**
     * Create a Surface that can be used as a target for blitting.
     *
     * Direct CPU access to the provided framebuffer remains possible,
     * but while a `Task` is active reads from and writes to the framebuffer
     * are undefined behaviour.
     */
    auto surface_for_fb(CPUAddressableDisplayAllocator::MappableFB const& fb) -> std::unique_ptr<Surface>;

    /**
     * Get a mapping of the provided buffer for CPU access.
     */
    auto map_buffer(Buffer const& buffer) -> std::unique_ptr<renderer::software::Mapping<std::byte const>>;
};

}

#endif // MIR_PLATFORM_GRAPHICS_RENDERING_PROVIDERS_H_
