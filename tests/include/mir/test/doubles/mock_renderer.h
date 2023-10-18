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
#ifndef MIR_TEST_DOUBLES_MOCK_RENDERER_H_
#define MIR_TEST_DOUBLES_MOCK_RENDERER_H_

#include "mir/renderer/renderer.h"
#include "mir/graphics/platform.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockRenderer : public renderer::Renderer
{
    MOCK_METHOD(void, set_viewport, (geometry::Rectangle const&));
    MOCK_METHOD(void, set_output_transform, (glm::mat2 const&));
    MOCK_METHOD(std::unique_ptr<graphics::Framebuffer>, render, (graphics::RenderableList const&), (const override));
    MOCK_METHOD(void, suspend, ());

    ~MockRenderer() noexcept {}
};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_RENDERER_H_ */
