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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "hwc_vsync.h"
namespace mga=mir::graphics::android;

mga::HWCVsync::HWCVsync()
    : vsync_occurred(false)
{
}

void mga::HWCVsync::wait_for_vsync()
{
    std::unique_lock<std::mutex> lk(vsync_wait_mutex);
    vsync_occurred = false;
    while(!vsync_occurred)
    {
        vsync_trigger.wait(lk);
    }
}

void mga::HWCVsync::notify_vsync()
{
    std::unique_lock<std::mutex> lk(vsync_wait_mutex);
    vsync_occurred = true;
    vsync_trigger.notify_all();
}
