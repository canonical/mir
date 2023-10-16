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

#ifndef MIR_TEST_DOUBLES_NULL_PLATFORM_H_
#define MIR_TEST_DOUBLES_NULL_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/test/doubles/null_display.h"

namespace mir
{
namespace test
{
namespace doubles
{
class NullDisplayPlatform : public graphics::DisplayPlatform
{
 public:
    auto create_display(
        std::shared_ptr<graphics::DisplayConfigurationPolicy> const&,
        std::shared_ptr<graphics::GLConfig> const&) -> mir::UniqueModulePtr<graphics::Display> override
    {
        return mir::make_module_ptr<NullDisplay>();
    }

protected:
    auto target_for()
        -> std::shared_ptr<graphics::DisplayTarget> override
    {
        return nullptr;
    }
};

class NullRenderingPlatform : public graphics::RenderingPlatform
{
public:
    auto create_buffer_allocator(graphics::Display const&)
        -> mir::UniqueModulePtr<graphics::GraphicBufferAllocator> override
    {
        return nullptr;
    }

protected:
    auto maybe_create_provider(
        graphics::RenderingProvider::Tag const&) -> std::shared_ptr<graphics::RenderingProvider> override
    {
        return nullptr;
    }
};
}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_NULL_PLATFORM_
