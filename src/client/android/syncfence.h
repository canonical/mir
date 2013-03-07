/*
 * Copyright © 2012 Canonical Ltd.
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
#ifndef MIR_CLIENT_ANDROID_SYNCFENCE_H_
#define MIR_CLIENT_ANDROID_SYNCFENCE_H_

#include <memory>

namespace mir
{
namespace client
{
namespace android
{

class IoctlWrapper
{
public:
    virtual int ioctl(int fd, unsigned long int request, int timeout) = 0;
    virtual int close(int fd) = 0;
};

class AndroidFence
{
public:
    virtual void wait() = 0;
};

class SyncFence : public AndroidFence
{
public:
    SyncFence(int /*fd*/);

    //override using real ioctl's with something else. useful for injecting mocks
    SyncFence(int /*fd*/, std::shared_ptr<IoctlWrapper> const& wrapper);

    void wait();

private:
    std::shared_ptr<IoctlWrapper> ioctl_wrapper;
};

}
}
}
#endif /* MIR_CLIENT_ANDROID_SYNCFENCE_H_ */
