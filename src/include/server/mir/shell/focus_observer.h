/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_SHELL_FOCUS_OBSERVER_H_
#define MIR_SHELL_FOCUS_OBSERVER_H_

#include <memory>

namespace mir
{
namespace scene { class Surface; }

namespace shell
{
/// Observer interface for focus changes
class FocusObserver
{
public:
    virtual ~FocusObserver() = default;
    
    /// Called when focus changes to a new surface
    /// @param surface The newly focused surface (may be nullptr if focus cleared)
    virtual void focus_changed(std::shared_ptr<scene::Surface> const& surface) = 0;
    
protected:
    FocusObserver() = default;
    FocusObserver(FocusObserver const&) = delete;
    FocusObserver& operator=(FocusObserver const&) = delete;
};
}
}

#endif // MIR_SHELL_FOCUS_OBSERVER_H_
