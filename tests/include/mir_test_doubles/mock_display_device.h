/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_MOCK_DISPLAY_DEVICE_H_
#define MIR_TEST_DOUBLES_MOCK_DISPLAY_DEVICE_H_

#include "mir/graphics/buffer.h"
#include "src/platforms/android/display_device.h"
#include "src/platforms/android/gl_context.h"
#include "src/platforms/android/hwc_fallback_gl_renderer.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
class MockDisplayDevice : public graphics::android::DisplayDevice
{
public:
    ~MockDisplayDevice() noexcept {}
    MOCK_METHOD0(content_cleared, void());
    MOCK_METHOD1(post_gl, void(graphics::android::SwappingGLContext const&));
    MOCK_METHOD3(post_overlays, bool(
        graphics::android::SwappingGLContext const&,
        graphics::RenderableList const&,
        graphics::android::RenderableListCompositor const&));
    MOCK_CONST_METHOD1(apply_orientation, bool(MirOrientation));
};
}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_DISPLAY_DEVICE_H_ */
