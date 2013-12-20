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
 */

#ifndef MIR_TEST_DOUBLES_STUB_DISPLAY_DEVICE_H_
#define MIR_TEST_DOUBLES_STUB_DISPLAY_DEVICE_H_

#include "src/platform/graphics/android/display_device.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubDisplayDevice : public graphics::android::DisplayDevice
{
    StubDisplayDevice()
    {
    }

    ~StubDisplayDevice() noexcept {}

    void mode(MirPowerMode)
    {
    }
    void prepare_composition()
    {
    }
    void gpu_render(EGLDisplay, EGLSurface)
    {
    }
    void post(graphics::Buffer const&)
    {
    }
};

}
}
}
#endif /* MIR_TEST_DOUBLES_STUB_DISPLAY_DEVICE_H_ */
