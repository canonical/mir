/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_HWC_COMPOSER_DEVICE_1_H_
#define MIR_TEST_DOUBLES_MOCK_HWC_COMPOSER_DEVICE_1_H_

#include <hardware/hwcomposer.h>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class ICSFBInterface
{
public:
    virtual int post_interface(struct framebuffer_device_t* dev, buffer_handle_t handle) = 0;
};

class MockFBHalDevice : public framebuffer_device_t, public ICSFBInterface
{
public:
    MockFBHalDevice()
    {
        using namespace testing;
        post = hook_post;
    }

    static int hook_post(struct framebuffer_device_t* mock_fb, buffer_handle_t handle)
    {
        MockFBHalDevice* mocker = static_cast<MockFBHalDevice*>(mock_fb);
        return mocker->post_interface(mock_fb, handle);
    }

    MOCK_METHOD2(post_interface, int(struct framebuffer_device_t*, buffer_handle_t));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_HWC_COMPOSER_DEVICE_1_H_ */
