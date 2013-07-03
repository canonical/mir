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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/asio_main_loop.h"
#include "src/server/graphics/gbm/udev_video_devices.h"

#include "mir_test_framework/udev_environment.h"

#include <gtest/gtest.h>

#include <thread>
#include <chrono>
#include <atomic>

namespace mgg = mir::graphics::gbm;
namespace mtf = mir::mir_test_framework;

/* TODO: Enable test when umockdev memory errors get fixed */
TEST(UdevVideoDevices, DISABLED_reports_change_in_video_devices)
{
    using namespace testing;

    char const* const devpath{"/sys/devices/pci0000:00/0000:00:01.0/0000:01:00.0/drm/card0"};
    int const expected_call_count{10};

    mtf::UdevEnvironment fake_devices;
    fake_devices.add_standard_drm_devices();

    auto udev_ctx = std::unique_ptr<udev,std::function<void(udev*)>>(
                        udev_new(),
                        [](udev* u) { if (u) udev_unref(u); });

    mgg::UdevVideoDevices video_devices{udev_ctx.get()};

    mir::AsioMainLoop ml;
    std::atomic<int> call_count{0};

    video_devices.register_change_handler(
        ml,
        [&call_count,&ml]
        {
            if (++call_count == expected_call_count)
                ml.stop();
        });

    std::thread t{
        [&fake_devices, devpath]
        {
            for (int i = 0; i < expected_call_count; ++i)
            {
                umockdev_testbed_uevent(fake_devices.testbed, devpath, "change");
                std::this_thread::sleep_for(std::chrono::microseconds{500});
            }
        }};

    ml.run();

    t.join();

    EXPECT_EQ(expected_call_count, call_count);
}
