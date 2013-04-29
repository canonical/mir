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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_INPUT_LISTENER_H_
#define MIR_TEST_DOUBLES_STUB_INPUT_LISTENER_H_

#include "mir/shell/input_target_listener.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubInputTargetListener : public shell::InputTargetListener
{
    void input_application_opened(std::shared_ptr<input::SessionTarget> const&)
    {
    }
    void input_application_closed(std::shared_ptr<input::SessionTarget> const&)
    {
    }
    void input_surface_opened(std::shared_ptr<input::SessionTarget> const&,
                              std::shared_ptr<input::SurfaceTarget> const&)
    {
    }
    void input_surface_closed(std::shared_ptr<input::SurfaceTarget> const&)
    {
    }
    void focus_changed(std::shared_ptr<input::SurfaceTarget> const&)
    {
    }
    
    void focus_cleared()
    {
    }
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_INPUT_LISTENER_H_
