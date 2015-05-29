/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_MOCK_RENDERABLE_LIST_COMPOSITOR_H_
#define MIR_TEST_DOUBLES_MOCK_RENDERABLE_LIST_COMPOSITOR_H_

#include "src/platforms/android/server/hwc_fallback_gl_renderer.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockRenderableListCompositor : public graphics::android::RenderableListCompositor
{
    MOCK_CONST_METHOD3(render,
        void(graphics::RenderableList const&, geometry::Displacement, graphics::android::SwappingGLContext const&));
};

}
}
}
#endif // MIR_TEST_DOUBLES_MOCK_RENDERABLE_LIST_COMPOSITOR_H_
