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

#ifndef MIR_TEST_DOUBLES_MOCK_ANDROID_REGISTRAR_H_
#define MIR_TEST_DOUBLES_MOCK_ANDROID_REGISTRAR_H_

#include <gmock/gmock.h>
#include "src/client/android/android_registrar.h"

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
struct MockAndroidRegistrar : public client::android::AndroidRegistrar
{
    ~MockAndroidRegistrar() noexcept {}
    MOCK_CONST_METHOD1(register_buffer,
        std::shared_ptr<const native_handle_t>(std::shared_ptr<MirBufferPackage> const&));
    MOCK_METHOD2(secure_for_cpu, std::shared_ptr<char>(std::shared_ptr<const native_handle_t>, geometry::Rectangle));
};
}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_ANDROID_REGISTRAR_H_ */
