/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#ifndef MIR_TEST_DOUBLES_STUB_GBM_NATIVE_BUFFER_H_
#define MIR_TEST_DOUBLES_STUB_GBM_NATIVE_BUFFER_H_

#include "src/platforms/mesa/server/common/gbm_buffer.h"
#include "mir/geometry/size.h"
#include <unistd.h>
#include <fcntl.h>
#include <gbm.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct StubGBMNativeBuffer : public graphics::mesa::GBMNativeBuffer
{
    StubGBMNativeBuffer(geometry::Size const& size, bool bypassable = true)
    {
        data_items = 4;
        //note: the client platform will only close the 1st fd, so lets not put more than one in here
        fd_items = 1;
        width = size.width.as_int();
        height = size.height.as_int();
        /*
         * From gbm.h
         *   GBM_BO_USE_SCANOUT      = (1 << 0),
         *   GBM_BO_USE_CURSOR_64X64 = (1 << 1),
         *   GBM_BO_USE_RENDERING    = (1 << 2),
         *   GBM_BO_USE_WRITE        = (1 << 3),
         */
        flags = (bypassable ? GBM_BO_USE_SCANOUT : 0) |
                              GBM_BO_USE_CURSOR_64X64 |
                              GBM_BO_USE_RENDERING;
        stride = 4390;
        bo = reinterpret_cast<gbm_bo*>(&fake_bo); //gbm_bo is opaque, so test code shouldn't dereference.
        for(auto i = 0; i < data_items; i++)
            data[i] = i;
        fd[0] = open("/dev/null", 0);
    }

    ~StubGBMNativeBuffer()
    {
        close(fd[0]);
    }

    int fake_bo{0}; 
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_GBM_NATIVE_BUFFER_H_ */
