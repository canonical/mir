/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
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
#ifndef MIR_GRAPHICS_ANDROID_SYNCFENCE_H_
#define MIR_GRAPHICS_ANDROID_SYNCFENCE_H_

#include "mir/graphics/android/sync_object.h"

namespace mir
{
namespace graphics
{
namespace android
{

class SyncFence : public SyncObject
{
public:
    SyncFence();
    SyncFence(int fd);
    ~SyncFence() noexcept;

    void wait();

private:
    SyncFence(SyncFence const&) = delete;
    SyncFence& operator=(SyncFence const&) = delete;
    int const fence_fd;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_SYNCFENCE_H_ */
