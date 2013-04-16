/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_GRAPHICS_DISPLAY_H_
#define MIR_GRAPHICS_DISPLAY_H_

#include "mir/graphics/viewable_area.h"

#include <memory>
#include <functional>

namespace mir
{

class MainLoop;

namespace graphics
{

class DisplayBuffer;
class DisplayConfiguration;

class Display : public ViewableArea
{
public:
    virtual geometry::Rectangle view_area() const = 0;
    virtual void for_each_display_buffer(std::function<void(DisplayBuffer&)> const& f) = 0;

    virtual std::shared_ptr<DisplayConfiguration> configuration() = 0;

    virtual void register_pause_resume_handlers(
        MainLoop& main_loop,
        std::function<void()> const& pause_handler,
        std::function<void()> const& resume_handler) = 0;

    virtual void pause() = 0;
    virtual void resume() = 0;

protected:
    Display() = default;
    ~Display() = default;
private:
    Display(Display const&) = delete;
    Display& operator=(Display const&) = delete;
};
}
}

#endif /* MIR_GRAPHICS_DISPLAY_H_ */
