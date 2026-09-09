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

#ifndef MIR_PLATFORM_GRAPHICS_DISPLAY_PROVIDERS_H_
#define MIR_PLATFORM_GRAPHICS_DISPLAY_PROVIDERS_H_

#include <mir/graphics/platform.h>
#include <vector>
#include <memory>
#include <span>
#include <cstdint>

namespace mir::graphics
{
class CPUAddressableDisplayProvider : public DisplayProvider
{
public:
    class Tag : public DisplayProvider::Tag
    {
    };
};

class CPUAddressableDisplayAllocator : public DisplayAllocator
{
public:
    class Tag : public DisplayAllocator::Tag
    {
    };

    class MappableFB : public Framebuffer, public mir::renderer::software::WriteMappable
    {
    public:
        MappableFB() = default;
        virtual ~MappableFB() override = default;

        using renderer::software::WriteMappable::size;
    };

    virtual auto supported_formats() const -> std::vector<DRMFormat> = 0;

    virtual auto alloc_fb(DRMFormat format) -> std::unique_ptr<MappableFB> = 0;

    virtual auto output_size() const -> geometry::Size = 0;
};

class GBMDisplayProvider : public DisplayProvider
{
public:
    class Tag : public DisplayProvider::Tag
    {
    };

    /**
    * Check if the provided UDev device is the same hardware device as this display
    *
    * This can be either because they point to the same device node, or because
    * the provided device is a Rendernode associated with the display hardware
    */
    virtual auto is_same_device(mir::udev::Device const& render_device) const -> bool = 0;

    /**
    * Check if this DisplaySink is driven by this DisplayProvider
    */
    virtual auto on_this_sink(DisplaySink& sink) const -> bool = 0;

    /**
    * Get the GBM device for this display
    */
    virtual auto gbm_device() const -> std::shared_ptr<struct gbm_device> = 0;
};

class GBMDisplayAllocator : public DisplayAllocator
{
public:
    class Tag : public DisplayAllocator::Tag
    {
    };

    /**
    * Formats supported for output
    */
    virtual auto supported_formats() const -> std::vector<DRMFormat> = 0;

    /**
    * Modifiers supported
    */
    virtual auto modifiers_for_format(DRMFormat format) const -> std::vector<uint64_t> = 0;

    class GBMSurface
    {
    public:
        GBMSurface() = default;
        virtual ~GBMSurface() = default;

        virtual operator gbm_surface*() const = 0;

        /**
        * Commit the current EGL front buffer as a KMS-displayable Framebuffer
        *
        * Like the underlying gbm_sufrace_lock_front_buffer GBM API, this
        * must be called after at least one call to eglSwapBuffers, and at most
        * once per eglSwapBuffers call.
        *
        * The Framebuffer should not be retained; a GBMSurface has a limited number
        * of buffers available and attempting to claim a framebuffer when no buffers
        * are free will result in an EBUSY std::system_error being raised.
        */
        virtual auto claim_framebuffer() -> std::unique_ptr<Framebuffer> = 0;
    };

    virtual auto make_surface(DRMFormat format, std::span<uint64_t> modifiers) -> std::unique_ptr<GBMSurface> = 0;
};

class DMABufBuffer;

class DmaBufDisplayAllocator : public DisplayAllocator
{
public:
    class Tag : public DisplayAllocator::Tag
    {
    };

    virtual auto framebuffer_for(std::shared_ptr<DMABufBuffer> buffer) -> std::unique_ptr<Framebuffer> = 0;
};

class GenericEGLDisplayProvider : public DisplayProvider
{
public:
    class Tag : public DisplayProvider::Tag
    {
    };

    virtual auto get_egl_display() -> EGLDisplay = 0;
};

class GenericEGLDisplayAllocator : public DisplayAllocator
{
public:
    class Tag : public DisplayAllocator::Tag
    {
    };

    class EGLFramebuffer : public graphics::Framebuffer
    {
    public:
        virtual void make_current() = 0;
        virtual void release_current() = 0;
        virtual auto clone_handle() -> std::unique_ptr<EGLFramebuffer> = 0;
    };

    virtual auto alloc_framebuffer(GLConfig const& config, EGLContext share_context)
        -> std::unique_ptr<EGLFramebuffer> = 0;
};
}

#endif // MIR_PLATFORM_GRAPHICS_DISPLAY_PROVIDERS_H_
