/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "src/server/graphics/android/hwc_vsync.h"
#include <gtest/gtest.h>
#include <thread>

namespace mga=mir::graphics::android;

class HWCVsync : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }
};

namespace
{
void* waiting_device(void* arg)
{
    mga::HWCVsync* vsync = (mga::HWCVsync*) arg;
    vsync->wait_for_vsync();
    return NULL;
}
}

TEST_F(HWCVsync, test_vsync_hook_waits)
{
    mga::HWCVsync hwc_vsync;

    pthread_t thread;
    pthread_create(&thread, NULL, waiting_device, (void*) &hwc_vsync);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    void* retval;
    auto error = pthread_tryjoin_np(thread, &retval);
    ASSERT_EQ(EBUSY, error);

    hwc_vsync.notify_vsync();
    error = pthread_join(thread, &retval);
    ASSERT_EQ(0, error);

}
