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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/test/doubles/stub_buffer.h"

#ifdef ANDROID
#include "mir/test/doubles/stub_android_native_buffer.h"
#elif defined(MESA_KMS) || defined(MESA_X11)
#include "mir/test/doubles/stub_gbm_native_buffer.h"
#endif

namespace mtd=mir::test::doubles;

auto mtd::StubBuffer::create_native_buffer()
-> std::shared_ptr<graphics::NativeBuffer>
{
#if defined(MESA_KMS) || defined(MESA_X11)
    return std::make_shared<StubGBMNativeBuffer>(geometry::Size{0,0});
#elif ANDROID
    return std::make_shared<StubAndroidNativeBuffer>();
#endif
}
