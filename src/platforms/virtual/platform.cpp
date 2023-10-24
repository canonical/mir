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
#include "display.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/egl_error.h"
#include "mir/log.h"
#include "options_parsing_helpers.h"
#include <drm_fourcc.h>

namespace mg = mir::graphics;
namespace mgv = mir::graphics::virt;
namespace geom = mir::geometry;
using namespace std::literals;


class mgv::Platform::VirtualDisplayInterfaceProvider : public mg::DisplayInterfaceProvider
{
public:
    explicit VirtualDisplayInterfaceProvider()
    {
    }

protected:
    auto maybe_create_interface(mg::DisplayProvider::Tag const& type_tag)
        -> std::shared_ptr<mg::DisplayProvider>
    {
        class VirtualEGLDisplayProvider : public GenericEGLDisplayProvider
        {
        public:

            explicit VirtualEGLDisplayProvider(EGLDisplay const egl_display) : egl_display{egl_display} {}

            auto get_egl_display() -> EGLDisplay override
            {
                return egl_display;
            }

            class StubEGLFramebuffer : public EGLFramebuffer
            {
            public:
                StubEGLFramebuffer() = default;
                StubEGLFramebuffer(StubEGLFramebuffer const&) {}
                void make_current() override {}
                void release_current() override {};
                auto clone_handle() -> std::unique_ptr<EGLFramebuffer>  override
                {
                    return std::make_unique<StubEGLFramebuffer>(*this);
                }
                auto size() const -> geom::Size override
                {
                    return geom::Size{};
                }
            };

            auto alloc_framebuffer(GLConfig const&, EGLContext) -> std::unique_ptr<EGLFramebuffer> override
            {
                return std::make_unique<StubEGLFramebuffer>();
            }

        private:
            EGLDisplay const egl_display;
        };

        class VirtualCPUAddressableDisplayProvider: public CPUAddressableDisplayProvider
        {
        public:
            class VirtualMappableFb: public MappableFB
            {
            public:
                VirtualMappableFb(geom::Size const& size, DRMFormat format)
                    : size_{size}, format_{format}
                {
                }

                auto format() const -> MirPixelFormat override
                {
                    return format_.as_mir_format().value_or(mir_pixel_format_invalid);
                }

                auto stride() const -> geom::Stride override
                {
                    return geom::Stride{};
                }

                auto size() const -> geom::Size override
                {
                    return size_;
                }

                auto map_writeable() -> std::unique_ptr<mir::renderer::software::Mapping<unsigned char>> override
                {
                    BOOST_THROW_EXCEPTION(std::logic_error("map_writeable is not implemented"));
                }

            private:
                geom::Size size_;
                DRMFormat format_;
            };

            auto supported_formats() const -> std::vector<DRMFormat> override
            {
                return {mg::DRMFormat{DRM_FORMAT_XRGB8888}, mg::DRMFormat{DRM_FORMAT_ARGB8888}};
            }

            auto alloc_fb(geometry::Size pixel_size, DRMFormat format) -> std::unique_ptr<MappableFB> override
            {
                return std::make_unique<VirtualMappableFb>(pixel_size, format);
            }
        };

        if (dynamic_cast<mg::GenericEGLDisplayProvider::Tag const*>(&type_tag))
        {
            auto const egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            if (egl_display == EGL_NO_DISPLAY)
            {
                log_info("Failed to create EGL display");
            }
            else if (eglInitialize(egl_display, nullptr, nullptr) == EGL_FALSE)
            {
                log_debug("Failed to initialise EGL: %s", mg::egl_category().message(eglGetError()).c_str());
            }
            else
            {
                return std::make_shared<VirtualEGLDisplayProvider>(egl_display);
            }
        }

        if (dynamic_cast<mg::CPUAddressableDisplayProvider::Tag const*>(&type_tag))
        {
            return std::make_shared<VirtualCPUAddressableDisplayProvider>();
        }

        return nullptr;
    }
};

mgv::Platform::Platform(
    std::shared_ptr<mg::DisplayReport> const& report,
    std::vector<VirtualOutputConfig> outputs)
    : report{report},
      provider{std::make_shared<VirtualDisplayInterfaceProvider>()},
      outputs{outputs}
{
}

mgv::Platform::~Platform()
{
}

mir::UniqueModulePtr<mg::Display> mgv::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const&,
    std::shared_ptr<GLConfig> const&)
{
    return mir::make_module_ptr<mgv::Display>(outputs);
}

auto mgv::Platform::interface_for() -> std::shared_ptr<DisplayInterfaceProvider>
{
    return provider;
}

auto mgv::Platform::parse_output_sizes(std::vector<std::string> virtual_outputs) -> std::vector<mgv::VirtualOutputConfig>
{
    std::vector<VirtualOutputConfig> configs;
    for (auto const& output : virtual_outputs)
    {
        std::vector<geom::Size > sizes;
        for (int start = 0, end; start - 1 < (int)output.size(); start = end + 1)
        {
            end = output.find(':', start);
            if (end == (int)std::string::npos)
                end = output.size();
            sizes.push_back(common::parse_size(output.substr(start, end - start)));
        }

        configs.push_back(VirtualOutputConfig(std::move(sizes)));
    }
    return configs;
}