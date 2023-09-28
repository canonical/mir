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

namespace mg = mir::graphics;
namespace mgv = mir::graphics::virt;
namespace geom = mir::geometry;
using namespace std::literals;

namespace
{
// TODO: This is copied from the X11 platform
auto parse_size_dimension(std::string const& str) -> int
{
    try
    {
        size_t num_end = 0;
        int const value = std::stoi(str, &num_end);
        if (num_end != str.size())
            BOOST_THROW_EXCEPTION(std::runtime_error("Output dimension \"" + str + "\" is not a valid number"));
        if (value <= 0)
            BOOST_THROW_EXCEPTION(std::runtime_error("Output dimensions must be greater than zero"));
        return value;
    }
    catch (std::invalid_argument const &)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Output dimension \"" + str + "\" is not a valid number"));
    }
    catch (std::out_of_range const &)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Output dimension \"" + str + "\" is out of range"));
    }
}

auto parse_size(std::string const& str) -> mgv::VirtualOutputConfig
{
    auto const x = str.find('x'); // "x" between width and height
    if (x == std::string::npos || x <= 0 || x >= str.size() - 1)
        BOOST_THROW_EXCEPTION(std::runtime_error("Output size \"" + str + "\" does not have two dimensions"));
    return mgv::VirtualOutputConfig{
        geom::Size{
            parse_size_dimension(str.substr(0, x)),
            parse_size_dimension(str.substr(x + 1))}};
}
}

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
                auto eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
                EGLint major, minor;
                if (eglInitialize(eglDisplay, &major, &minor) == EGL_FALSE)
                    BOOST_THROW_EXCEPTION(std::runtime_error("Failed to initialize EGL display"));
                return eglDisplay;
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
    std::vector<VirtualOutputConfig> output_sizes)
    : report{report},
      provider{std::make_shared<VirtualDisplayInterfaceProvider>()},
      output_sizes{output_sizes}
{
}

mir::UniqueModulePtr<mg::Display> mgv::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const&,
    std::shared_ptr<GLConfig> const&)
{
    return mir::make_module_ptr<mgv::Display>(output_sizes);
}

auto mgv::Platform::interface_for() -> std::shared_ptr<DisplayInterfaceProvider>
{
    return provider;
}

auto mgv::Platform::parse_output_sizes(std::string output_sizes) -> std::vector<mgv::VirtualOutputConfig>
{
    std::vector<mgv::VirtualOutputConfig> sizes;
    for (int start = 0, end; start - 1 < (int)output_sizes.size(); start = end + 1)
    {
        end = output_sizes.find(':', start);
        if (end == (int)std::string::npos)
            end = output_sizes.size();
        sizes.push_back(parse_size(output_sizes.substr(start, end - start)));
    }
    return sizes;
}