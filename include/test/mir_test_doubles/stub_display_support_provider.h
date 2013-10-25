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
 */

#ifndef MIR_TEST_DOUBLES_STUB_DISPLAY_SUPPORT_PROVIDER_H_
#define MIR_TEST_DOUBLES_STUB_DISPLAY_SUPPORT_PROVIDER_H_

#include "src/server/graphics/android/display_support_provider.h"
 
namespace mir
{
namespace test
{
namespace doubles
{

struct StubDisplaySupportProvider : public graphics::android::DisplaySupportProvider
{
    StubDisplaySupportProvider(geometry::Size sz)
     : sz(sz)
    {
    }
    StubDisplaySupportProvider()
     : StubDisplaySupportProvider({0,0})
    {
    }

    ~StubDisplaySupportProvider() noexcept {}

    geometry::Size display_size() const { return sz; }
    geometry::PixelFormat display_format() const { return geometry::PixelFormat(); }
    unsigned int number_of_framebuffers_available() const { return 0; }
    void set_next_frontbuffer(std::shared_ptr<mir::graphics::Buffer> const&) {}
    void sync_to_display(bool) {}
    void mode(MirPowerMode) {}
    void commit_frame(EGLDisplay, EGLSurface) {}
private:
    geometry::Size sz;
};

}
}
}
#endif /* MIR_TEST_DOUBLES_STUB_DISPLAY_SUPPORT_PROVIDER_H_ */
