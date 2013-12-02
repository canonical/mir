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

#ifndef MIR_TEST_DOUBLES_STUB_DISPLAY_BUILDER_H_
#define MIR_TEST_DOUBLES_STUB_DISPLAY_BUILDER_H_

#include "src/platform/graphics/android/display_builder.h"
#include "stub_display_buffer.h"
#include "stub_display_device.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct StubDisplayBuilder : public graphics::android::DisplayBuilder
{
    StubDisplayBuilder(std::shared_ptr<graphics::android::DisplayDevice> const& stub_dev, geometry::Size sz)
        : stub_dev(stub_dev), sz(sz)
    {
    }

    StubDisplayBuilder()
        : StubDisplayBuilder(std::make_shared<StubDisplayDevice>(), geometry::Size{0,0})
    {
    }

    StubDisplayBuilder(geometry::Size sz)
        : StubDisplayBuilder(std::make_shared<StubDisplayDevice>(), sz)
    {
    }

    StubDisplayBuilder(std::shared_ptr<graphics::android::DisplayDevice> const& stub_dev)
        : stub_dev(stub_dev), sz(geometry::Size{0,0})
    {
    }

    MirPixelFormat display_format()
    {
        return mir_pixel_format_abgr_8888;
    }

    std::unique_ptr<graphics::DisplayBuffer> create_display_buffer(
        std::shared_ptr<graphics::android::DisplayDevice> const&,
        graphics::android::GLContext const&)
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

#endif /* MIR_TEST_DOUBLES_STUB_DISPLAY_BUILDER_H_ */
