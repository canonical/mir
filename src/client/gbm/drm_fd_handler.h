/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_CLIENT_GBM_DRM_FD_HANDLER_
#define MIR_CLIENT_GBM_DRM_FD_HANDLER_

#include <sys/types.h>

namespace mir
{
namespace client
{
namespace gbm
{

class DRMFDHandler
{
public:
    virtual ~DRMFDHandler() {}

    virtual int ioctl(unsigned long request, void* arg) = 0;
    virtual int primeFDToHandle(int prime_fd, uint32_t *handle) = 0;
    virtual int close(int fd) = 0;
    virtual void* map(size_t size, off_t offset) = 0;
    virtual void unmap(void* addr, size_t size) = 0;

protected:
    DRMFDHandler() = default;
    DRMFDHandler(const DRMFDHandler&) = delete;
    DRMFDHandler& operator=(const DRMFDHandler&) = delete;
};

}
}
}

#endif /* MIR_CLIENT_GBM_DRM_FD_HANDLER_ */
