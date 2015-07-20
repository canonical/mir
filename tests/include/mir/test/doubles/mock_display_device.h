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
#include "src/platforms/android/server/display_device.h"
#include "src/platforms/android/server/gl_context.h"
#include "src/platforms/android/server/hwc_fallback_gl_renderer.h"
#include "src/platforms/android/server/hwc_layerlist.h"
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
    MOCK_METHOD1(commit, void(std::list<graphics::android::DisplayContents> const&));
    MOCK_METHOD1(compatible_renderlist, bool(
        graphics::RenderableList const&));
    MOCK_CONST_METHOD0(recommended_sleep, std::chrono::milliseconds());
};
}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_DISPLAY_DEVICE_H_ */
