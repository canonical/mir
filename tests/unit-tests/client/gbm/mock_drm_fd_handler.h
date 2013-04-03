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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_CLIENT_MOCK_DRM_FD_HANDLER_H_
#define MIR_TEST_CLIENT_MOCK_DRM_FD_HANDLER_H_

#include <gmock/gmock.h>

#include "src/client/gbm/drm_fd_handler.h"

namespace mir
{
namespace client
{
namespace gbm
{

class MockDRMFDHandler : public DRMFDHandler
{
public:
    MOCK_METHOD2(ioctl, int(unsigned long request, void* arg));
    MOCK_METHOD2(primeFDToHandle, int(int prime_fd, uint32_t *handle));
    MOCK_METHOD1(close, int(int fd));
    MOCK_METHOD2(map, void*(size_t size, off_t offset));
    MOCK_METHOD2(unmap, void(void* addr, size_t size));
};

}
}
}
#endif
