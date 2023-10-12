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
#include "options_parsing_helpers.h"

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
    auto maybe_create_interface(mg::DisplayInterfaceBase::Tag const& type_tag)
        -> std::shared_ptr<mg::DisplayInterfaceBase>
    {
        class StubGenericEGLDisplayProvider : public GenericEGLDisplayProvider
        {
        public:
            auto get_egl_display() -> EGLDisplay override
            {
                auto egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
                EGLint major, minor;
                if (eglInitialize(egl_display, &major, &minor) == EGL_FALSE)
                    BOOST_THROW_EXCEPTION(std::runtime_error("Failed to initialize EGL display"));
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
        };

        if (dynamic_cast<mg::GenericEGLDisplayProvider::Tag const*>(&type_tag))
        {
            return std::make_shared<StubGenericEGLDisplayProvider>();
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