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

#include "src/platform/graphics/mesa/gbm_buffer.h"
#include "mir/geometry/size.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubGBMNativeBuffer : public graphics::mesa::GBMNativeBuffer
{
    StubGBMNativeBuffer(geometry::Size const& size)
    {
        data_items = 4;
        fd_items = 2;
        width = size.width.as_int();
        height = size.height.as_int();
        flags = 0x66;
        stride = 4390;
        bo = reinterpret_cast<gbm_bo*>(&fake_bo); //gbm_bo is opaque, so test code shouldn't dereference.
        for(auto i = 0; i < data_items; i++)
            data[i] = i;
        for(auto i = 0; i < fd_items; i++)
            fd[i] = i;
    }
    int fake_bo{0}; 
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_GBM_NATIVE_BUFFER_H_ */
