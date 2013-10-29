/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_STUB_DISPLAY_BUFFER_FACTORY_H_
#define MIR_TEST_DOUBLES_STUB_DISPLAY_BUFFER_FACTORY_H_

#include "src/server/graphics/android/android_display_buffer_factory.h"
#include "stub_display_buffer.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct StubDisplayBufferFactory : public graphics::android::AndroidDisplayBufferFactory
{
    StubDisplayBufferFactory()
    {
    }

    StubDisplayBufferFactory(std::shared_ptr<graphics::android::DisplayDevice> const& stub_dev)
        : stub_dev(stub_dev)
    {
    }

    StubDisplayBufferFactory(geometry::Size sz)
        : sz(sz)
    {
    }

    std::unique_ptr<graphics::DisplayBuffer> create_display_buffer(
        std::shared_ptr<graphics::android::DisplayDevice> const&,
        EGLDisplay, EGLConfig, EGLContext)
    {
        return std::unique_ptr<graphics::DisplayBuffer>(
                new StubDisplayBuffer(geometry::Rectangle{{0,0},sz}));
    }
    
    std::shared_ptr<graphics::android::DisplayDevice> create_display_device()
    {
        return stub_dev;
    }

    std::shared_ptr<graphics::android::DisplayDevice> const stub_dev;
    geometry::Size sz;
};
}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_STUB_DISPLAY_BUFFER_FACTORY_H_ */
