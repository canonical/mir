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

#ifndef MIR_TEST_DOUBLES_MOCK_FB_HAL_DEVICE_H_
#define MIR_TEST_DOUBLES_MOCK_FB_HAL_DEVICE_H_

#include <hardware/hwcomposer.h>
#include <hardware/gralloc.h>
#include <hardware/fb.h>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockFBHalDevice : public framebuffer_device_t
{
public:
    MockFBHalDevice(unsigned int const width, unsigned int const height,
                    int const pf, int const numfbs)
        : framebuffer_device_t({
            hw_device_t(),
            0,
            width,
            height,
            0,
            pf,
            0.0f,
            0.0f,
            0.0f,
            0,
            1,
            numfbs,
            {0,0,0,0,0,0,0},
            nullptr, nullptr,nullptr,nullptr, nullptr,nullptr,
            {0,0,0,0,0,0}
          })
    {
        post = hook_post;
        setSwapInterval = hook_setSwapInterval;
        enableScreen = hook_enableScreen;
    }

    MockFBHalDevice()
        : MockFBHalDevice(1,1,1,1)
    {
    }

    static int hook_post(struct framebuffer_device_t* mock_fb, buffer_handle_t handle)
    {
        MockFBHalDevice* mocker = static_cast<MockFBHalDevice*>(mock_fb);
        return mocker->post_interface(mock_fb, handle);
    }

    static int hook_setSwapInterval(struct framebuffer_device_t* mock_fb, int interval)
    {
        MockFBHalDevice* mocker = static_cast<MockFBHalDevice*>(mock_fb);
        return mocker->setSwapInterval_interface(mock_fb, interval);
    }

    static int hook_enableScreen(struct framebuffer_device_t* mock_fb, int enable)
    {
        MockFBHalDevice* mocker = static_cast<MockFBHalDevice*>(mock_fb);
        return mocker->enableScreen_interface(mock_fb, enable);
    }


    MOCK_METHOD2(enableScreen_interface, int(struct framebuffer_device_t*, int));
    MOCK_METHOD2(post_interface, int(struct framebuffer_device_t*, buffer_handle_t));
    MOCK_METHOD2(setSwapInterval_interface, int(struct framebuffer_device_t*, int));
};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_FB_HAL_DEVICE_H_ */
