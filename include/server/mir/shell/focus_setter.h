/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_SHELL_FOCUS_SETTER_H_
#define MIR_SHELL_FOCUS_SETTER_H_

#include <memory>

namespace mir
{

namespace shell
{
class Session;

/// Interface used by the Shell to propagate changes in the focus model to interested views
/// e.g. Input, or Surfaces.
class FocusSetter
{
public:
    virtual ~FocusSetter() {}

    virtual void set_focus_to(std::shared_ptr<shell::Session> const& new_focus) = 0;

protected:
    FocusSetter() = default;
    FocusSetter(const FocusSetter&) = delete;
    FocusSetter& operator=(const FocusSetter&) = delete;
};

}
}


#endif // MIR_SHELL_FOCUS_SETTER_H_
