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
#ifndef MIR_GRAPHICS_ANDROID_SYNC_FENCE_H_
#define MIR_GRAPHICS_ANDROID_SYNC_FENCE_H_

#include "mir/graphics/android/fence.h"
#include <memory>

namespace mir
{
namespace graphics
{
namespace android
{

class SyncFileOps
{
public:
    virtual ~SyncFileOps() = default;
    virtual int ioctl(int, int, void*) = 0;
    virtual int dup(int) = 0;
    virtual int close(int) = 0;
};

class RealSyncFileOps : public SyncFileOps
{
public:
    int ioctl(int fd, int req, void* dat);
    int dup(int fd);
    int close(int fd);
};

class SyncFence : public Fence
{
public:
    SyncFence(std::shared_ptr<SyncFileOps> const&, int fd);
    ~SyncFence() noexcept;

    void wait();
    void merge_with(NativeFence& merge_fd);
    NativeFence copy_native_handle() const;

private:
    SyncFence(SyncFence const&) = delete;
    SyncFence& operator=(SyncFence const&) = delete;

    int fence_fd;
    std::shared_ptr<SyncFileOps> const ops;

    int const infinite_timeout = -1;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_SYNC_FENCE_H_ */
