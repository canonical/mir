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

#ifndef MIR_TEST_DOUBLES_STUB_DISPLAY_COMMANDER_H_
#define MIR_TEST_DOUBLES_STUB_DISPLAY_COMMANDER_H_

#include "src/server/graphics/android/display_commander.h"
 
namespace mir
{
namespace test
{
namespace doubles
{

struct StubDisplayCommand : public graphics::android::DisplayCommander
{
    StubDisplayCommand()
    {
    }
    ~StubDisplayCommand() noexcept {}

    void set_next_frontbuffer(std::shared_ptr<mir::graphics::Buffer> const&) {}
    void sync_to_display(bool) {}
    void mode(MirPowerMode) {}
    void commit_frame(EGLDisplay, EGLSurface) {}
};

}
}
}
#endif /* MIR_TEST_DOUBLES_STUB_DISPLAY_COMMANDER_H_ */
