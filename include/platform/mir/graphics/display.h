/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_GRAPHICS_DISPLAY_H_
#define MIR_GRAPHICS_DISPLAY_H_

#include <memory>
#include <functional>

namespace mir
{

namespace graphics
{

class DisplayBuffer;
class DisplayConfiguration;
class Cursor;
class GLContext;
class EventHandlerRegister;

typedef std::function<bool()> DisplayPauseHandler;
typedef std::function<bool()> DisplayResumeHandler;
typedef std::function<void()> DisplayConfigurationChangeHandler;

/**
 * Interface to the display subsystem.
 */
class Display
{
public:
    /**
     * Executes a functor for each output framebuffer.
     */
    virtual void for_each_display_buffer(std::function<void(DisplayBuffer&)> const& f) = 0;

    /**
     * Gets a copy of the current output configuration.
     */
    virtual std::unique_ptr<DisplayConfiguration> configuration() const = 0;

    /**
     * Sets a new output configuration.
     */
    virtual void configure(DisplayConfiguration const& conf) = 0;

    /**
     * Registers a handler for display configuration changes.
     *
     * Note that the handler is called only for hardware changes (e.g. monitor
     * plugged/unplugged), not for changes initiated by software (e.g. modesetting).
     *
     * The implementation should use the functionality provided by the MainLoop
     * to register the handlers in a way appropriate for the platform.
     */
    virtual void register_configuration_change_handler(
        EventHandlerRegister& handlers,
        DisplayConfigurationChangeHandler const& conf_change_handler) = 0;

    /**
     * Registers handlers for pausing and resuming the display subsystem.
     *
     * The implementation should use the functionality provided by the EventHandlerRegister
     * to register the handlers in a way appropriate for the platform.
     */
    virtual void register_pause_resume_handlers(
        EventHandlerRegister& handlers,
        DisplayPauseHandler const& pause_handler,
        DisplayResumeHandler const& resume_handler) = 0;

    /**
     * Pauses the display.
     *
     * This method may temporarily (until resumed) release any resources
     * associated with the display subsystem.
     */
    virtual void pause() = 0;

    /**
     * Resumes the display.
     */
    virtual void resume() = 0;

    /**
     * Gets the hardware cursor object.
     */
    virtual std::weak_ptr<Cursor> the_cursor() = 0;

    /**
     * Creates a GLContext object that shares resources with the Display's GL context.
     *
     * This is usually implemented as a shared EGL context. This object can be used
     * to access graphics resources from an arbitrary thread.
     */
    virtual std::unique_ptr<GLContext> create_gl_context() = 0;

protected:
    Display() = default;
    virtual ~Display() {/* TODO: make nothrow */}
private:
    Display(Display const&) = delete;
    Display& operator=(Display const&) = delete;
};
}
}

#endif /* MIR_GRAPHICS_DISPLAY_H_ */
