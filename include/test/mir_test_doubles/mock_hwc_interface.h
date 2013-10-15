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

#ifndef MIR_TEST_DOUBLES_MOCK_HWC_INTERFACE_H_
#define MIR_TEST_DOUBLES_MOCK_HWC_INTERFACE_H_

#include "src/server/graphics/android/hwc_device.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockHWCInterface : public graphics::android::HWCDevice
{
public:
    ~MockHWCInterface() noexcept {}
    MOCK_METHOD0(wait_for_vsync, void());
    MOCK_METHOD2(commit_frame, void(EGLDisplay, EGLSurface));
    MOCK_CONST_METHOD0(display_size, geometry::Size());
    MOCK_CONST_METHOD0(display_format, geometry::PixelFormat());
    MOCK_CONST_METHOD0(number_of_framebuffers_available, unsigned int());
    MOCK_METHOD1(set_next_frontbuffer, void(std::shared_ptr<mir::graphics::Buffer> const&));
    MOCK_METHOD1(sync_to_display, void(bool));
    MOCK_METHOD1(mode, void(MirPowerMode));
};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_HWC_INTERFACE_H_ */
