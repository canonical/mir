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

#ifndef MIR_GRAPHICS_PLATFORM_H_
#define MIR_GRAPHICS_PLATFORM_H_

#include <boost/program_options/options_description.hpp>
#include <any>
#include <span>
#include <gbm.h>

#include "mir/graphics/drm_formats.h"
#include "mir/module_properties.h"
#include "mir/module_deleter.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/fd.h"

#include <EGL/egl.h>

namespace mir
{
class EmergencyCleanupRegistry;
class ConsoleServices;

namespace frontend
{
class Surface;
}
namespace options
{
class Option;
}
namespace renderer::software
{
class WriteMappableBuffer;
}

namespace udev
{
class Context;
class Device;
}

/// Graphics subsystem. Mediates interaction between core system and
/// the graphics environment.
namespace graphics
{
class Buffer;
class Framebuffer;
class Display;
class DisplaySink;
class DisplayReport;
class DisplayConfigurationPolicy;
class GraphicBufferAllocator;
class GLConfig;

namespace probe
{
    /**
     * A measure of how well a platform supports a device
     *
     * \note This is compared as an integer; best + 1 is a valid Result that
     *       will be used in preference to a module that reports best.
     *       Platform modules distributed with Mir will never use a priority higher
     *       than best.
     */
    using Result = uint32_t;
    Result const unsupported = 0;    /**< Unable to function at all on this device */
    Result const dummy = 1;          /**< Used only for dummy or stub platforms.
                                      */
    Result const supported = 128;    /**< Capable of providing a functionality on this device;
                                      *   possibly with degraded performance or features.
                                      */
    Result const nested = 192;       /**< Capable of providing fully-featured functionality on this device;
                                      *   running nested under some other display server rather than with
                                      *   exclusive hardware access.
                                      */
    Result const best = 256;         /**< Capable of providing the best features and performance this device
                                      *   is capable of.
                                      */
}

class RenderingProvider
{
public:
    class Tag
    {
    public:
        Tag() = default;
        virtual ~Tag() = default;
    };

    virtual ~RenderingProvider() = default;

    class FramebufferProvider
    {
    public:
        FramebufferProvider() = default;
        virtual ~FramebufferProvider() = default;

        FramebufferProvider(FramebufferProvider const&) = delete;
        auto operator=(FramebufferProvider const&) = delete;

        /**
         * Get a directly-displayable handle for a buffer, if cheaply possible
         *
         * Some buffer types can be passed as-is to the display hardware. If this
         * buffer can be used in this way (on the DisplayInterfaceProvider associated
         * with this FramebufferProvider), this method creates a handle that can be
         * passed to the overlay method of an associated DisplaySink.
         *
         * \note    The returned Framebuffer may share ownership of the provided Buffer.
         *          It is not necessary for calling code to retain a reference to the Buffer.
         * \param buffer
         * \return  A handle to a directly submittable buffer, or nullptr if this buffer
                    is not pasable as-is to the display hardware.
         */
        virtual auto buffer_to_framebuffer(std::shared_ptr<Buffer> buffer)
            -> std::unique_ptr<Framebuffer> = 0;
    };

    /**
     * Check how well this Renderer can support a particular display sink
     */
    virtual auto suitability_for_display(DisplaySink& sink)
        -> probe::Result = 0;

    /**
     * Check how well this Renderer can support a particular BufferAllocator
     */
    virtual auto suitability_for_allocator(std::shared_ptr<GraphicBufferAllocator> const& target)
        -> probe::Result = 0;

    virtual auto make_framebuffer_provider(DisplaySink& sink)
        -> std::unique_ptr<FramebufferProvider> = 0;
};

namespace gl
{
class Texture;
class OutputSurface;
}

class GLRenderingProvider : public RenderingProvider
{
public:
    class Tag : public RenderingProvider::Tag
    {
    };

    virtual auto as_texture(std::shared_ptr<Buffer> buffer) -> std::shared_ptr<gl::Texture> = 0;

    virtual auto surface_for_sink(
        DisplaySink& sink,
        GLConfig const& config) -> std::unique_ptr<gl::OutputSurface> = 0;
};

namespace drm
{
class Syncobj;
}

class DRMRenderingProvider : public GLRenderingProvider
{
public:
    class Tag : public RenderingProvider::Tag
    {
    };

    virtual auto import_syncobj(mir::Fd const& syncobj_fd) -> std::unique_ptr<drm::Syncobj> = 0;
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
    virtual UniqueModulePtr<GraphicBufferAllocator> create_buffer_allocator(Display const& output) = 0;

    /**
     * Attempt to acquire a platform-specific interface from this RenderingPlatform
     *
     * Any given platform is not guaranteed to implement any specific interface,
     * and the set of supported interfaces may depend on the runtime environment.
     *
     * Since this may result in a runtime probe the call may be costly, and the
     * result should be saved rather than re-acquiring an interface each time.
     *
     * \tparam Provider
     * \return  On success: an occupied std::shared_ptr<Provider>
     *          On failure: std::shared_ptr<Provider>{nullptr}
     */
    template<typename Provider>
    static auto acquire_provider(std::shared_ptr<RenderingPlatform> platform) -> std::shared_ptr<Provider>
    {
        static_assert(
            std::is_convertible_v<Provider*, RenderingProvider*>,
            "Can only acquire a Renderer interface; Provider must implement RenderingProvider");

        if (auto const provider = platform->maybe_create_provider(typename Provider::Tag{}))
        {
            if (auto const requested = std::dynamic_pointer_cast<Provider>(provider))
            {
                return requested;
            }
            BOOST_THROW_EXCEPTION((
                std::logic_error{
                    "Implementation error! Platform returned object that does not support requested interface"}));
        }
        return nullptr;
    }

    virtual auto driver_name() -> std::string const&
    {
        static std::string unimpl = "unimplemented";
        return unimpl;
    }

protected:
    /**
     * Acquire a specific rendering interface
     *
     * This should perform any runtime checks necessary to verify the requested interface is
     * expected to work and return a pointer to an implementation of that interface.
     *
     * This function is guaranteed to be called with `this` managed by a `shared_ptr`; if
     * the returned value needs to share ownership with `this`, calls to std::shared_from_this
     * can be expected to work.
     *
     * \param type_tag  [in]    An instance of the Tag type for the requested interface.
     *                          Implementations are expected to dynamic_cast<> this to
     *                          discover the specific interface being requested.
     * \return  On success: A pointer to an implementation of the RenderingProvider-derived
     *                      interface that corresponds to the most-derived type of tag_type
     *          On failure: std::shared_ptr<RenderingProvider>{nullptr}
     */
    virtual auto maybe_create_provider(
        RenderingProvider::Tag const& type_tag) -> std::shared_ptr<RenderingProvider> = 0;
};

class DisplayProvider
{
public:
    class Tag
    {
    public:
        Tag() = default;
        virtual ~Tag() = default;
    };

    virtual ~DisplayProvider() = default;
};


class DisplayAllocator
{
public:
    class Tag
    {
    public:
        Tag() = default;
        virtual ~Tag() = default;
    };

    virtual ~DisplayAllocator() = default;
};

class Framebuffer
{
public:
    Framebuffer() = default;
    virtual ~Framebuffer() = default;

    /**
     * The size of this framebuffer, in pixels
     */
    virtual auto size() const -> geometry::Size = 0;
};

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

    class MappableFB :
        public Framebuffer,
        public mir::renderer::software::WriteMappableBuffer
    {
    public:
        MappableFB() = default;
        virtual ~MappableFB() override = default;

        using renderer::software::WriteMappableBuffer::size;
    };

    virtual auto supported_formats() const
        -> std::vector<DRMFormat> = 0;

    virtual auto alloc_fb(DRMFormat format)
        -> std::unique_ptr<MappableFB> = 0;

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

    virtual auto framebuffer_for(std::shared_ptr<DMABufBuffer> buffer)
        -> std::unique_ptr<Framebuffer> = 0;
};

#ifndef EGLStreamKHR
typedef void* EGLStreamKHR;
#endif

class EGLStreamDisplayProvider : public DisplayProvider
{
public:
    class Tag : public DisplayProvider::Tag
    {
    };

    virtual auto get_egl_display() const -> EGLDisplay = 0;
};

class EGLStreamDisplayAllocator : public DisplayAllocator
{
public:
    class Tag : public DisplayAllocator::Tag
    {
    };

    virtual auto claim_stream() -> EGLStreamKHR = 0;
    virtual auto output_size() const -> geometry::Size = 0;
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

    virtual auto alloc_framebuffer(GLConfig const& config, EGLContext share_context) -> std::unique_ptr<EGLFramebuffer> = 0;
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
     * Attempt to acquire a platform-specific interface from this DisplayPlatform
     *
     * Any given platform is not guaranteed to implement any specific interface,
     * and the set of supported interfaces may depend on the runtime environment.
     *
     * Since this may result in a runtime probe the call may be costly, and the
     * result should be saved rather than re-acquiring an interface each time.
     *
     * \tparam Interface
     * \return  On success: an occupied std::shared_ptr<Interface>
     *          On failure: std::shared_ptr<Interface>{nullptr}
     */
    template<typename Interface>
    auto acquire_provider() -> std::shared_ptr<Interface>
    {
        static_assert(
            std::is_convertible_v<Interface*, DisplayProvider*>,
            "Can only acquire a Display interface; Interface must implement DisplayProvider");

        if (auto const base_interface = maybe_create_provider(typename Interface::Tag{}))
        {
            if (auto const requested_interface = std::dynamic_pointer_cast<Interface>(base_interface))
            {
                return requested_interface;
            }
            BOOST_THROW_EXCEPTION((std::logic_error{
                "Implementation error! Platform returned object that does not support requested interface"}));
        }
        return nullptr;
    }

protected:
    /**
     * Acquire a specific hardware interface
     *
     * This should perform any runtime checks necessary to verify the requested interface is
     * expected to work and return a pointer to an implementation of that interface.
     *
     * \param type_tag  [in]    An instance of the Tag type for the requested interface.
     *                          Implementations are expected to dynamic_cast<> this to
     *                          discover the specific interface being requested.
     * \return      A pointer to an implementation of the DisplayProvider-derived
     *              interface that corresponds to the most-derived type of tag_type.
     */
    virtual auto maybe_create_provider(DisplayProvider::Tag const& type_tag)
        -> std::shared_ptr<DisplayProvider> = 0;
};

struct SupportedDevice
{
    /**
     * UDev device the platform claims
     *
     * Mir assumes that all devices that platforms can claim are represented in udev,
     * and can be uniquely identified there. Particularly: platform loading will guarantee
     * that at most one platform (of each type) is created on each unique device.
     *
     * \note    It is acceptable for device to be empty; in this case Mir assumes that
     *          device is incomparable with any other device. In particular, hosted platforms
     *          like X11 or Wayland should use an empty device, as they are not bound to any
     *          particular hardware.
     */
    std::unique_ptr<udev::Device> device;
    probe::Result support_level;     /**< How well the platform can support this device */

    /**
     * Platform-private data from probing
     *
     * If there is any extra data helpful in creating a Platform on this device, the platform can
     * stash it here, and it will be passed in to the platform constructor.
     */
    std::any platform_data;
};

typedef mir::UniqueModulePtr<mir::graphics::DisplayPlatform>(*CreateDisplayPlatform)(
    mir::graphics::SupportedDevice const& device,
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mir::graphics::DisplayReport> const& report);

typedef mir::UniqueModulePtr<mir::graphics::RenderingPlatform>(*CreateRenderPlatform)(
    mir::graphics::SupportedDevice const& device,
    std::vector<std::shared_ptr<mir::graphics::DisplayPlatform>> const& platforms,
    mir::options::Option const& options,
    mir::EmergencyCleanupRegistry& emergency_cleanup_registry);

typedef void(*AddPlatformOptions)(
    boost::program_options::options_description& config);

typedef std::vector<mir::graphics::SupportedDevice>(*PlatformProbe)(
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mir::udev::Context> const&,
    mir::options::Option const& options);

typedef std::vector<mir::graphics::SupportedDevice>(*RenderProbe)(
    std::span<std::shared_ptr<mir::graphics::DisplayPlatform>> const&,
    mir::ConsoleServices&,
    std::shared_ptr<mir::udev::Context> const&,
    mir::options::Option const&);

typedef mir::ModuleProperties const*(*DescribeModule)();
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
mir::UniqueModulePtr<mir::graphics::DisplayPlatform> create_display_platform(
    mir::graphics::SupportedDevice const& device,
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mir::graphics::DisplayReport> const& report);

mir::UniqueModulePtr<mir::graphics::RenderingPlatform> create_rendering_platform(
    mir::graphics::SupportedDevice const& device,
    std::vector<std::shared_ptr<mir::graphics::DisplayPlatform>> const& targets,
    mir::options::Option const& options,
    mir::EmergencyCleanupRegistry& emergency_cleanup_registry);

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


auto probe_display_platform(
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mir::udev::Context> const& udev,
    mir::options::Option const& options) -> std::vector<mir::graphics::SupportedDevice>;

auto probe_rendering_platform(
    std::span<std::shared_ptr<mir::graphics::DisplayPlatform>> const& targets,
    mir::ConsoleServices& console,
    std::shared_ptr<mir::udev::Context> const& udev,
    mir::options::Option const& options) -> std::vector<mir::graphics::SupportedDevice>;

mir::ModuleProperties const* describe_graphics_module();

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
}

#endif // MIR_GRAPHICS_PLATFORM_H_
