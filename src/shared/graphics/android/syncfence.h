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
#ifndef MIR_GRAPHICS_ANDROID_SYNCFENCE_H_
#define MIR_GRAPHICS_ANDROID_SYNCFENCE_H_

#include <memory>

#define SYNC_IOC_WAIT 0x40043E00

namespace mir
{
namespace client
{
namespace android
{

class IoctlWrapper
{
public:
    virtual ~IoctlWrapper() {}
    virtual int ioctl(int fd, unsigned long int request, int* timeout) const = 0;
    virtual int close(int fd) const = 0;

protected:
    IoctlWrapper() = default;
    IoctlWrapper(IoctlWrapper const&) = delete;
    IoctlWrapper& operator=(IoctlWrapper const&) = delete;
};

class AndroidFence
{
public:
    virtual ~AndroidFence() {}
    virtual void wait() = 0;

protected:
    AndroidFence() = default;
    AndroidFence(AndroidFence const&) = delete;
    AndroidFence& operator=(AndroidFence const&) = delete;
};

class SyncFence : public AndroidFence
{
public:
    SyncFence(int fd, std::shared_ptr<IoctlWrapper> const& wrapper);
    ~SyncFence();

    void wait();

private:
    SyncFence(SyncFence const&) = delete;
    SyncFence& operator=(SyncFence const&) = delete;

    std::shared_ptr<IoctlWrapper> const ioctl_wrapper;
    int const fence_fd;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_SYNCFENCE_H_ */
