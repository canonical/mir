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

#ifndef MIR_INPUT_NULL_INPUT_TARGETER_H_
#define MIR_INPUT_NULL_INPUT_TARGETER_H_

#include "mir/shell/input_targeter.h"

namespace mir
{
namespace input
{

class NullInputTargeter : public shell::InputTargeter
{
public:
    NullInputTargeter() {};
    virtual ~NullInputTargeter() noexcept(true) {}
    
    virtual void focus_changed(std::shared_ptr<input::SurfaceTarget const> const&)
    {
    }
    virtual void focus_cleared()
    {
    }

protected:
    NullInputTargeter(const NullInputTargeter&) = delete;
    NullInputTargeter& operator=(const NullInputTargeter&) = delete;
};

}
}

#endif // MIR_INPUT_NULL_INPUT_TARGETER_H_
