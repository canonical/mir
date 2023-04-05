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

#ifndef MIR_FRONTEND_DRAG_ICON_CONTROLLER_H_
#define MIR_FRONTEND_DRAG_ICON_CONTROLLER_H_

#include <memory>

namespace mir
{
namespace scene { class Surface; }

namespace frontend
{

/// An interface for associating an image with the cursor
class DragIconController
{
public:
    virtual ~DragIconController() = default;

    virtual void set_drag_icon(std::weak_ptr<scene::Surface> icon) = 0;

protected:
    DragIconController() = default;
    DragIconController(DragIconController const&) = delete;
    DragIconController& operator=(DragIconController const&) = delete;
};

}
}

#endif // MIR_FRONTEND_DRAG_ICON_CONTROLLER_H_
