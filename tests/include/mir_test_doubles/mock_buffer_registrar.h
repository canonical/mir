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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_BUFFER_REGISTRAR_H_
#define MIR_TEST_DOUBLES_MOCK_BUFFER_REGISTRAR_H_

#include "src/platforms/android/client/buffer_registrar.h"
#include <gmock/gmock.h>

namespace mir
{
namespace geometry
{
class Rectangle;
}
namespace test
{
namespace doubles
{
struct MockBufferRegistrar : public client::android::BufferRegistrar
{
    ~MockBufferRegistrar() noexcept {}
    MOCK_CONST_METHOD2(register_buffer,
        std::shared_ptr<graphics::NativeBuffer>(MirBufferPackage const&,
        MirPixelFormat));
    MOCK_METHOD2(secure_for_cpu, std::shared_ptr<char>(
        std::shared_ptr<graphics::NativeBuffer> const&,
        geometry::Rectangle const));
};
}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_BUFFER_REGISTRAR_H_ */
