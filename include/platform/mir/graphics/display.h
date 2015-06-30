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
#include <chrono>

namespace mir
{

namespace graphics
{

class DisplayBuffer;
class DisplayConfiguration;
class Cursor;
class CursorImage;
class GLContext;
class EventHandlerRegister;

typedef std::function<bool()> DisplayPauseHandler;
typedef std::function<bool()> DisplayResumeHandler;
typedef std::function<void()> DisplayConfigurationChangeHandler;

/**
 * DisplaySyncGroup represents a group of displays that need to be output
 * in unison as a single post() call.
 * This is only appropriate for platforms whose post() calls are non-blocking
 * and not synchronous with the screen hardware (e.g. virtual machines or
 * Android).
 * Using a DisplaySyncGroup with multiple screens on a platform whose post()
 * blocks for vsync often results in stuttering, and so should be avoided.
 * Although using DisplaySyncGroup with a single DisplayBuffer remains safe
 * for any platform.
 */
class DisplaySyncGroup
{
public:
    /**
     *  Executes a functor that allows the DisplayBuffer contents to be updated
    **/
    virtual void for_each_display_buffer(std::function<void(DisplayBuffer&)> const& f) = 0;

    /** Post the content of the DisplayBuffers associated with this DisplaySyncGroup.
     *  The content of all the DisplayBuffers in this DisplaySyncGroup are guaranteed to be onscreen
     *  in the near future. On some platforms, this may wait a potentially long time for vsync. 
    **/
    virtual void post() = 0;

    /**
     * Returns a recommendation to the compositor as to how long it should
     * wait before sampling the scene for the next frame. Sampling the
     * scene too early results in up to one whole frame of extra lag if
     * rendering is fast or skipped altogether (bypass/overlays). But sampling
     * too late and we might miss the deadline. If unsure just return zero.
     */
    virtual std::chrono::milliseconds recommended_sleep() const = 0;

    virtual ~DisplaySyncGroup() = default;
protected:
    DisplaySyncGroup() = default;
    DisplaySyncGroup(DisplaySyncGroup const&) = delete;
    DisplaySyncGroup& operator=(DisplaySyncGroup const&) = delete;
};

/**
 * Interface to the display subsystem.
 */
class Display
{
public:
    /**
     * Executes a functor for each output group.
     */
    virtual void for_each_display_sync_group(std::function<void(DisplaySyncGroup&)> const& f) = 0;

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
     * Create a hardware cursor object.
     */
    virtual std::shared_ptr<Cursor> create_hardware_cursor(std::shared_ptr<CursorImage> const& initial_image) = 0;

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
