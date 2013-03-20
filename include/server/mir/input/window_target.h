/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_WINDOW_TARGET_H_
#define MIR_INPUT_WINDOW_TARGET_H_

#include "mir/geometry/size.h"

#include <string>

namespace mir
{
namespace input
{

class WindowTarget
{
public:
    virtual ~WindowTarget() {}
    
    virtual geometry::Size size() const = 0;
    virtual std::string name() const = 0;
    
    virtual int server_input_fd() const = 0;    

protected:
    WindowTarget() = default;
    WindowTarget(WindowTarget const&) = delete;
    WindowTarget& operator=(WindowTarget const&) = delete;
};

}
} // namespace mir

#endif // MIR_INPUT_WINDOW_TARGET_H_
