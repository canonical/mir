/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_TEST_DOUBLES_MOCK_SURFACE_RENDERER_H_
#define MIR_TEST_DOUBLES_MOCK_SURFACE_RENDERER_H_

#include "mir/graphics/renderable.h"
#include "mir/graphics/renderer.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSurfaceRenderer : public graphics::Renderer
{
    MOCK_METHOD2(render, void(std::function<void(std::shared_ptr<void> const&)>, graphics::Renderable&));
};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_SURFACE_RENDERER_H_ */
