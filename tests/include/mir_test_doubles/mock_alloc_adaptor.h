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

#ifndef MIR_TEST_DOUBLES_MOCK_ALLOC_ADAPTOR_H_
#define MIR_TEST_DOUBLES_MOCK_ALLOC_ADAPTOR_H_

#include "src/platforms/android/server/graphic_alloc_adaptor.h"

#include <system/window.h>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockAllocAdaptor : public graphics::android::GraphicAllocAdaptor
{
public:
    MOCK_METHOD3(alloc_buffer,
        std::shared_ptr<graphics::NativeBuffer>(geometry::Size, MirPixelFormat, graphics::android::BufferUsage));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_ALLOC_ADAPTOR_H_ */
