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

#include "mir/graphics/frame.h"
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
class EventHandlerRegister;
class VirtualOutput;

typedef std::function<bool()> DisplayPauseHandler;
typedef std::function<bool()> DisplayResumeHandler;
typedef std::function<void()> DisplayConfigurationChangeHandler;

class NativeDisplay
{
protected:
    NativeDisplay() = default;
    virtual ~NativeDisplay() = default;
    NativeDisplay(NativeDisplay const&) = delete;
    NativeDisplay operator=(NativeDisplay const&) = delete;
};

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
     *
     * This is equivalent to:
     * https://www.opengl.org/registry/specs/NV/glx_delay_before_swap.txt
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
     * Applying a display configuration only if it will not invalidate existing DisplayBuffers
     *
     * The Display must guarantee that the references to the DisplayBuffer acquired via
     * DisplaySyncGroup::for_each_display_buffer() remain valid until the Display is destroyed or
     * Display::configure() is called.
     *
     * If this function returns \c true then the new display configuration has been applied.
     * If this function returns \c false then the new display configuration has not been applied.
     *
     * In either case this function guarantees that existing DisplayBuffer references will remain
     * valid.
     *
     * \param conf [in] Configuration to possibly apply.
     * \return      \c true if \p conf has been applied as the new output configuration.
     */
    virtual bool apply_if_configuration_preserves_display_buffers(DisplayConfiguration const& conf) = 0;

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
     * Creates a virtual output
     *  \returns null if the implementation does not support virtual outputs
     */
    virtual std::unique_ptr<VirtualOutput> create_virtual_output(int width, int height) = 0;

    /** Returns a pointer to the native display object backing this
     *  display.
     *
     *  The pointer to the native display remains valid as long as the
     *  display object is valid.
     */
    virtual NativeDisplay* native_display() = 0;

    /**
     * Returns timing information for the last frame displayed on a given
     * output.
     *
     * Frame timing will be provided to clients only when they request it.
     * This is to ensure idle clients never get woken by unwanted events.
     * It is also distinctly separate from the display configuration as this
     * timing information changes many times per second and should not interfere
     * with the more static display configuration.
     *
     * Note: Using unsigned here because DisplayConfigurationOutputId is
     * troublesome (can't be forward declared) and including
     * display_configuration.h to get it would be an overkill.
     */
    virtual Frame last_frame_on(unsigned output_id) const = 0;

    Display() = default;
    virtual ~Display() = default;
private:
    Display(Display const&) = delete;
    Display& operator=(Display const&) = delete;
};
}
}

#endif /* MIR_GRAPHICS_DISPLAY_H_ */
