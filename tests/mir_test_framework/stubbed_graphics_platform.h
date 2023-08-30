/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_FRAMEWORK_STUBBED_GRAPHICS_PLATFORM_H_
#define MIR_TEST_FRAMEWORK_STUBBED_GRAPHICS_PLATFORM_H_

#include "mir/test/doubles/null_platform.h"

namespace mir_test_framework
{
class StubGraphicPlatform :
    public mir::test::doubles::NullDisplayPlatform,
    public mir::test::doubles::NullRenderingPlatform
{
public:
    StubGraphicPlatform(std::vector<mir::geometry::Rectangle> const& display_rects);

    mir::UniqueModulePtr<mir::graphics::GraphicBufferAllocator> create_buffer_allocator(
        mir::graphics::Display const& output) override;

    mir::UniqueModulePtr<mir::graphics::Display> create_display(
        std::shared_ptr<mir::graphics::DisplayConfigurationPolicy> const&,
        std::shared_ptr<mir::graphics::GLConfig> const&,
        std::shared_ptr<mir::options::Option> const&) override;

protected:
    auto maybe_create_interface(
        mir::graphics::RendererInterfaceBase::Tag const& tag)
        -> std::shared_ptr<mir::graphics::RendererInterfaceBase> override;

    auto interface_for()
        -> std::shared_ptr<mir::graphics::DisplayInterfaceProvider> override;
private:
    std::vector<mir::geometry::Rectangle> const display_rects;
};
}

#endif /* MIR_TEST_FRAMEWORK_STUBBED_GRAPHICS_PLATFORM_H_ */
