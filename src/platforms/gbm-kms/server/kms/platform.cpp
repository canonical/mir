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

#include "platform.h"
#include "buffer_allocator.h"
#include "display.h"
#include "mir/console_services.h"
#include "mir/emergency_cleanup_registry.h"
#include "mir/graphics/dmabuf_buffer.h"
#include "mir/graphics/drm_formats.h"
#include "mir/graphics/egl_context_executor.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/texture.h"
#include "mir/udev/wrapper.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/egl_extensions.h"
#include "one_shot_device_observer.h"
#include "mir/graphics/linux_dmabuf.h"
#include "mir/graphics/egl_context_executor.h"
#include "kms_cpu_addressable_display_provider.h"
#include "surfaceless_egl_context.h"
#include <boost/throw_exception.hpp>
#include <drm_fourcc.h>
#include <gbm.h>
#include <system_error>

#define MIR_LOG_COMPONENT "platform-graphics-gbm-kms"
#include "mir/log.h"

#include <fcntl.h>
#include <boost/exception/all.hpp>

namespace mg = mir::graphics;
namespace mgg = mg::gbm;

namespace
{
auto master_fd_for_device(mir::udev::Device const& device, mir::ConsoleServices& vt) -> std::tuple<std::unique_ptr<mir::Device>, mir::Fd>
{
    mir::Fd drm_fd;
    auto device_handle = vt.acquire_device(
        major(device.devnum()), minor(device.devnum()),
        std::make_unique<mg::common::OneShotDeviceObserver>(drm_fd))
            .get();

    if (drm_fd == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to acquire DRM fd"}));
    }
    
    return std::make_tuple(std::move(device_handle), std::move(drm_fd));
}

auto maybe_make_gbm_provider(mir::Fd drm_fd) -> std::shared_ptr<mgg::GBMDisplayProvider>
{
    try
    {
        return std::make_shared<mgg::GBMDisplayProvider>(std::move(drm_fd));
    }
    catch (std::exception const& err)
    {
        mir::log_info("Failed to create GBM device for direct buffer submission");
        mir::log_info("Output will use CPU buffer copies");
        return {};
    }
}
}

mgg::Platform::Platform(
    udev::Device const& device,
    std::shared_ptr<DisplayReport> const& listener,
    ConsoleServices& vt,
    EmergencyCleanupRegistry& registry,
    BypassOption bypass_option)
    : Platform(master_fd_for_device(device, vt), listener, registry, bypass_option)
{
}

mgg::Platform::Platform(
    std::tuple<std::unique_ptr<Device>, mir::Fd> drm,
    std::shared_ptr<DisplayReport> const& listener,
    EmergencyCleanupRegistry&,
    BypassOption bypass_option)
    : udev{std::make_shared<mir::udev::Context>()},
      listener{listener},
      device_handle{std::move(std::get<0>(drm))},
      drm_fd{std::move(std::get<1>(drm))},
      gbm_display_provider{maybe_make_gbm_provider(drm_fd)},
      bypass_option_{bypass_option}
{
    if (drm_fd == mir::Fd::invalid)
    {
        BOOST_THROW_EXCEPTION((std::logic_error{"Invalid DRM device FD"}));
    }

    /*
     * This is required to pull the list of output colour formats available,
     * and universal planes support was unconditionally enabled in the 3.16 kernel.
     * That's long enough ago to just unconditionally enable this.
     */
    if (auto err = drmSetClientCap(drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1))
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                -err,
                std::system_category(),
                "Failed to enable DRM Universal Planes support"}));
    }
}

mgg::Platform::~Platform() = default;

namespace
{
auto gbm_device_for_udev_device(
    mir::udev::Device const& device,
    std::vector<std::shared_ptr<mg::DisplayPlatform>> const& displays)
     -> std::variant<std::shared_ptr<mg::GBMDisplayProvider>, std::shared_ptr<gbm_device>>
{
    /* First check to see whether our device exactly matches a display device.
     * If so, we should use its GBM device
     */
    for(auto const& display_device : displays)
    {
        if (auto gbm_display = display_device->acquire_provider<mg::GBMDisplayProvider>())
        {
            if (gbm_display->is_same_device(device))
            {
                return gbm_display;
            }
        }
    }

    // We don't match any display HW, create our own GBM device
    if (auto node = device.devnode())
    {
        auto fd = mir::Fd{open(node, O_RDWR | O_CLOEXEC)};
        if (fd < 0)
        {
            BOOST_THROW_EXCEPTION((
                std::system_error{
                    errno,
                    std::system_category(),
                    "Failed to open DRM device"}));
        }
        std::shared_ptr<gbm_device> gbm{
            gbm_create_device(fd),
            [fd = std::move(fd)](gbm_device* device)
            {
                if (device)
                {
                    gbm_device_destroy(device);
                }
            }};
        if (!gbm)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to create GBM device"}));
        }
        return gbm;
    }

    BOOST_THROW_EXCEPTION((
        std::runtime_error{"Attempt to create GBM device from UDev device with no device node?!"}));
}

/**
 * Initialise an EGLDisplay and return the initialised display
 */
auto initialise_egl(EGLDisplay dpy, int minimum_major_version, int minimum_minor_version) -> EGLDisplay
{
    EGLint major, minor;

    if (eglInitialize(dpy, &major, &minor) == EGL_FALSE)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to initialize EGL display"));

    if ((major < minimum_major_version) ||
        (major == minimum_major_version && minor < minimum_minor_version))
    {
        std::stringstream msg;
        msg << "Incompatible EGL version. Requested: "
            << minimum_major_version << "." << minimum_minor_version
            << " got: " << major << "." << minor;
        BOOST_THROW_EXCEPTION((std::runtime_error{msg.str()}));
    }

    return dpy;
}

auto dpy_for_gbm_device(gbm_device* device) -> EGLDisplay
{
    mg::EGLExtensions::PlatformBaseEXT platform_ext;

    auto const egl_display = platform_ext.eglGetPlatformDisplay(
        EGL_PLATFORM_GBM_KHR,      // EGL_PLATFORM_GBM_MESA has the same value.
        static_cast<EGLNativeDisplayType>(device),
        nullptr);
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to get EGL display"));

    return egl_display;
}

struct display_provider_or_nothing
{
    auto operator()(std::shared_ptr<mg::GBMDisplayProvider> provider) { return provider; }
    auto operator()(std::shared_ptr<gbm_device>) { return std::shared_ptr<mg::GBMDisplayProvider>{}; }
};

struct gbm_device_from_hw
{
    auto operator()(std::shared_ptr<mg::GBMDisplayProvider> const& provider) { return provider->gbm_device(); }
    auto operator()(std::shared_ptr<gbm_device> device) { return device; }    
};

class DMABufBuffer : public mg::DMABufBuffer
{
public:
    DMABufBuffer(
        mg::DRMFormat format,
        std::optional<uint64_t> modifier,
        std::vector<PlaneDescriptor> planes,
        mg::gl::Texture::Layout layout,
        mir::geometry::Size size)
        : format_{format},
          modifier_{modifier},
          planes_{std::move(planes)},
          layout_{layout},
          size_{size}
    {
    }

    auto format() const -> mg::DRMFormat override
    {
        return format_;
    }
    auto modifier() const -> std::optional<uint64_t> override
    {
        return modifier_;
    }
    auto planes() const -> std::vector<PlaneDescriptor> const& override
    {
        return planes_;
    }
    auto layout() const -> mg::gl::Texture::Layout override
    {
        return layout_;
    }
    auto size() const -> mir::geometry::Size override
    {
        return size_;
    }
private:
    mg::DRMFormat format_;
    std::optional<uint64_t> modifier_;
    std::vector<PlaneDescriptor> planes_;
    mg::gl::Texture::Layout layout_;
    mir::geometry::Size size_;
};

auto gbm_bo_with_modifiers_or_linear(
    gbm_device* gbm,
    mg::DRMFormat format,
    std::span<uint64_t const> modifiers,
    mir::geometry::Size size) -> std::unique_ptr<struct gbm_bo, decltype(&gbm_bo_destroy)>
{
    errno = 0;
    auto gbm_bo = gbm_bo_create_with_modifiers2(
        gbm,
        size.width.as_uint32_t(), size.height.as_uint32_t(),
        format,
        modifiers.data(), modifiers.size(),
        GBM_BO_USE_RENDERING);
    if (!gbm_bo && errno != ENOSYS)
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                errno,
                std::system_category(),
                "Failed to allocate GBM bo"}));
    }
    else if (!gbm_bo)
    {
        // We get ENOSYS if the GBM implementation can't handle modifiers
        if (std::find(modifiers.begin(), modifiers.end(), DRM_FORMAT_MOD_LINEAR) == modifiers.end())
        {
            // Shouldn't happen, but if LINEAR isn't one of the requested modifiers we can't allocate something compatible
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to allocate GBM bo: non-linear modifier requested, but implementation does not support modifiers"}));
        }

        gbm_bo = gbm_bo_create(
            gbm,
            size.width.as_uint32_t(), size.height.as_uint32_t(),
            format,
            GBM_BO_USE_RENDERING | GBM_BO_USE_LINEAR);
        if (!gbm_bo)
        {
            BOOST_THROW_EXCEPTION((
                std::system_error{
                    errno,
                    std::system_category(),
                    "Failed to allocate GBM bo without modifiers"}));
        }
    }

    return {gbm_bo, &gbm_bo_destroy};
}

auto alloc_dma_buf(
    gbm_device* gbm,
    mg::DRMFormat format,
    std::span<uint64_t const> modifiers,
    mir::geometry::Size size) -> std::shared_ptr<mg::DMABufBuffer>
{
    auto gbm_bo = gbm_bo_with_modifiers_or_linear(gbm, format, modifiers, size);

    auto plane_count = gbm_bo_get_plane_count(gbm_bo.get());
    std::vector<mg::DMABufBuffer::PlaneDescriptor> planes;
    planes.reserve(plane_count);

    for (int i = 0; i < plane_count; ++i)
    {
        planes.push_back(
            mg::DMABufBuffer::PlaneDescriptor {
                .dma_buf = mir::Fd{gbm_bo_get_fd_for_plane(gbm_bo.get(), i)},
                .stride = gbm_bo_get_stride_for_plane(gbm_bo.get(), i),
                .offset = gbm_bo_get_offset(gbm_bo.get(), i)
            });
    }

    std::optional<uint64_t> modifier;
    auto raw_modifier = gbm_bo_get_modifier(gbm_bo.get());
    if (raw_modifier == DRM_FORMAT_MOD_INVALID)
    {
        modifier = std::nullopt;
    }
    else
    {
        modifier = raw_modifier;
    }

    return std::make_shared<DMABufBuffer>(
        format,
        modifier,
        std::move(planes),
        mg::gl::Texture::Layout::GL,
        size);
}

auto maybe_make_dmabuf_provider(
    std::shared_ptr<gbm_device> gbm,
    EGLDisplay dpy,
    std::shared_ptr<mg::EGLExtensions> egl_extensions,
    std::shared_ptr<mg::common::EGLContextExecutor> egl_delegate)
    -> std::shared_ptr<mg::DMABufEGLProvider>
{
    try
    {
        mg::EGLExtensions::EXTImageDmaBufImportModifiers modifier_ext{dpy};
        return std::make_shared<mg::DMABufEGLProvider>(
            dpy,
            std::move(egl_extensions),
            modifier_ext,
            std::move(egl_delegate),
            [gbm](mg::DRMFormat format, std::span<uint64_t const> modifiers, mir::geometry::Size size)
                -> std::shared_ptr<mg::DMABufBuffer>
            {
                return alloc_dma_buf(gbm.get(), format, modifiers, size);
            });
    }
    catch (std::runtime_error const& error)
    {
        mir::log_info(
            "Cannot enable linux-dmabuf import support: %s", error.what());
        mir::log(
            mir::logging::Severity::debug,
            MIR_LOG_COMPONENT,
            std::current_exception(),
            "Detailed error: ");
    }
    return nullptr;
}
}

mgg::RenderingPlatform::RenderingPlatform(
    mir::udev::Device const& device,
    std::vector<std::shared_ptr<mg::DisplayPlatform>> const& platforms)
    : RenderingPlatform(gbm_device_for_udev_device(device, platforms))
{
}

mgg::RenderingPlatform::RenderingPlatform(
    std::variant<std::shared_ptr<mg::GBMDisplayProvider>, std::shared_ptr<gbm_device>> hw)
    : device{std::visit(gbm_device_from_hw{}, hw)},
      bound_display{std::visit(display_provider_or_nothing{}, hw)},
      share_ctx{std::make_unique<SurfacelessEGLContext>(initialise_egl(dpy_for_gbm_device(device.get()), 1, 4))},
      egl_delegate{std::make_shared<mg::common::EGLContextExecutor>(share_ctx->make_share_context())},
      dmabuf_provider{maybe_make_dmabuf_provider(device, share_ctx->egl_display(), std::make_shared<mg::EGLExtensions>(), egl_delegate)}
{
}

mgg::RenderingPlatform::~RenderingPlatform() = default;

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgg::RenderingPlatform::create_buffer_allocator(
    mg::Display const&)
{
    return make_module_ptr<mgg::BufferAllocator>(
        std::make_unique<SurfacelessEGLContext>(share_ctx->egl_display(), static_cast<EGLContext>(*share_ctx)),
        egl_delegate,
        dmabuf_provider);
}

auto mgg::RenderingPlatform::maybe_create_provider(
    RenderingProvider::Tag const& type_tag) -> std::shared_ptr<RenderingProvider>
{
    if (dynamic_cast<GLRenderingProvider::Tag const*>(&type_tag))
    {
        return std::make_shared<mgg::GLRenderingProvider>(
            bound_display,
            egl_delegate,
            dmabuf_provider,
            share_ctx->egl_display(),
            static_cast<EGLContext>(*share_ctx));
    }
    return nullptr;
}

namespace
{
auto gbm_device_from_provider(std::shared_ptr<mg::GBMDisplayProvider> const& provider)
    -> std::shared_ptr<struct gbm_device>
{
    if (provider)
    {
        return provider->gbm_device();
    }
    return nullptr;
}
}

mir::UniqueModulePtr<mg::Display> mgg::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy, std::shared_ptr<GLConfig> const&)
{
    return make_module_ptr<mgg::Display>(
        drm_fd,
        gbm_device_from_provider(gbm_display_provider),
        bypass_option_,
        initial_conf_policy,
        listener);
}

auto mgg::Platform::maybe_create_provider(DisplayProvider::Tag const& type_tag)
    -> std::shared_ptr<DisplayProvider>
{
    if (dynamic_cast<mg::GBMDisplayProvider::Tag const*>(&type_tag))
    {
        return gbm_display_provider;
    }
    if (dynamic_cast<mg::CPUAddressableDisplayProvider::Tag const*>(&type_tag))
    {
        /* There's no implementation behind it, but we want to know during probe time
         * that the DisplayBuffers will support it.
         */
        return std::make_shared<CPUAddressableDisplayProvider>();
    }
    return {};
}

mgg::BypassOption mgg::Platform::bypass_option() const
{
    return bypass_option_;
}
