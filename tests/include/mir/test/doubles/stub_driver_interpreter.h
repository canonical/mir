/*
 * Copyright © 2012 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_STUB_DRIVER_INTERPRETER_H_
#define MIR_TEST_DOUBLES_STUB_DRIVER_INTERPRETER_H_

#include "mir/graphics/android/android_driver_interpreter.h"

namespace mir
{
namespace test
{
namespace doubles
{

class StubDriverInterpreter : public graphics::android::AndroidDriverInterpreter
{
public:
    StubDriverInterpreter(mir::geometry::Size sz, int visual_id)
     : sz{sz},
       visual_id{visual_id}
    {
    }

    StubDriverInterpreter()
     : StubDriverInterpreter(mir::geometry::Size{44,22}, 5)
    {
    }

    mir::graphics::NativeBuffer* driver_requests_buffer() override
    {
        return nullptr;
    }

    void driver_returns_buffer(ANativeWindowBuffer*, int)
    {
    }

    void dispatch_driver_request_format(int) override
    {
    }

    int driver_requests_info(int index) const override
    {
        if (index == NATIVE_WINDOW_WIDTH)
            return sz.width.as_uint32_t();
        if (index == NATIVE_WINDOW_HEIGHT)
            return sz.height.as_uint32_t();
        if (index == NATIVE_WINDOW_FORMAT)
            return visual_id;
        return 0;
    }

    void sync_to_display(bool) override
    {
    }

    void dispatch_driver_request_buffer_count(unsigned int) override
    {
    } 
private:
    mir::geometry::Size sz;
    int visual_id;
};
}
}
}
#endif /* MIR_TEST_DOUBLES_STUB_BUFFER_H_ */
