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

#ifndef MIR_TEST_DOUBLES_MOCK_RENDERER_FACTORY_H
#define MIR_TEST_DOUBLES_MOCK_RENDERER_FACTORY_H

#include "mir/renderer/renderer_factory.h"

namespace mir
{
namespace test
{
namespace doubles
{
class MockRendererFactory : public renderer::RendererFactory
{
public:
    MOCK_METHOD(
        std::unique_ptr<renderer::Renderer>,
        create_renderer_for,
        (std::unique_ptr<graphics::gl::OutputSurface>, std::shared_ptr<graphics::GLRenderingProvider>),
        (const override));
};
}
}
}

#endif //MIR_TEST_DOUBLES_MOCK_RENDERER_FACTORY_H
