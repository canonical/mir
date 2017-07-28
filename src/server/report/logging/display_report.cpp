/*
 * Copyright © 2012 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "display_report.h"
#include "mir/logging/logger.h"
#include <EGL/eglext.h>
#include <sstream>
#include <cstring>
#include <cstdlib>

namespace ml=mir::logging;
namespace mrl=mir::report::logging;

mrl::DisplayReport::DisplayReport(
    std::shared_ptr<ml::Logger> const& logger)
    : logger(logger)
{
}

mrl::DisplayReport::~DisplayReport()
{
}

const char* mrl::DisplayReport::component()
{
    static const char* s = "graphics";
    return s;
}


void mrl::DisplayReport::report_successful_setup_of_native_resources()
{
    logger->log(ml::Severity::informational, "Successfully setup native resources.", component());
}

void mrl::DisplayReport::report_successful_egl_make_current_on_construction()
{
    logger->log(ml::Severity::informational, "Successfully made egl context current on construction.", component());
}

void mrl::DisplayReport::report_successful_egl_buffer_swap_on_construction()
{
    logger->log(ml::Severity::informational, "Successfully performed egl buffer swap on construction.", component());
}

void mrl::DisplayReport::report_successful_drm_mode_set_crtc_on_construction()
{
    logger->log(ml::Severity::informational, "Successfully performed drm mode setup on construction.", component());
}

void mrl::DisplayReport::report_successful_display_construction()
{
    logger->log(ml::Severity::informational, "Successfully finished construction.", component());
}

void mrl::DisplayReport::report_drm_master_failure(int error)
{
    std::stringstream ss;
    ss << "Failed to change ownership of DRM master (error: " << strerror(error) << ").";
    if (error == EPERM || error == EACCES)
        ss << " Try running Mir with root privileges.";

    logger->log(ml::Severity::warning, ss.str(), component());
}

void mrl::DisplayReport::report_vt_switch_away_failure()
{
    logger->log(ml::Severity::warning, "Failed to switch away from Mir VT.", component());
}

void mrl::DisplayReport::report_vt_switch_back_failure()
{
    logger->log(ml::Severity::warning, "Failed to switch back to Mir VT.", component());
}

void mrl::DisplayReport::report_egl_configuration(EGLDisplay disp, EGLConfig config)
{
    auto ext = eglQueryString(disp, EGL_EXTENSIONS);
    std::string extensions { ext ? ext : "" };
    logger->log(ml::Severity::informational, "Display EGL Extensions: " + extensions, component());

    auto client_ext = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (client_ext)
    {
        logger->log(ml::Severity::informational,
            "EGL_EXT_client_extensions: " + std::string{client_ext}, component());
    }
    else
    {
        logger->log(ml::Severity::informational, "EGL_EXT_client_extensions not supported", component());
        //clear out error
        eglGetError();
    }

    #define STRMACRO(X) {#X, X}
    struct {std::string name; EGLint val;} egl_string_mapping [] =
    {
        STRMACRO(EGL_BUFFER_SIZE),
        STRMACRO(EGL_ALPHA_SIZE),
        STRMACRO(EGL_BLUE_SIZE),
        STRMACRO(EGL_GREEN_SIZE),
        STRMACRO(EGL_RED_SIZE),
        STRMACRO(EGL_DEPTH_SIZE),
        STRMACRO(EGL_STENCIL_SIZE),
        STRMACRO(EGL_CONFIG_CAVEAT),
        STRMACRO(EGL_CONFIG_ID),
        STRMACRO(EGL_LEVEL),
        STRMACRO(EGL_MAX_PBUFFER_HEIGHT),
        STRMACRO(EGL_MAX_PBUFFER_PIXELS),
        STRMACRO(EGL_MAX_PBUFFER_WIDTH),
        STRMACRO(EGL_NATIVE_RENDERABLE),
        STRMACRO(EGL_NATIVE_VISUAL_ID),
        STRMACRO(EGL_NATIVE_VISUAL_TYPE),
        STRMACRO(EGL_SAMPLES),
        STRMACRO(EGL_SAMPLE_BUFFERS),
        STRMACRO(EGL_SURFACE_TYPE),
        STRMACRO(EGL_TRANSPARENT_TYPE),
        STRMACRO(EGL_TRANSPARENT_BLUE_VALUE),
        STRMACRO(EGL_TRANSPARENT_GREEN_VALUE),
        STRMACRO(EGL_TRANSPARENT_RED_VALUE),
        STRMACRO(EGL_BIND_TO_TEXTURE_RGB),
        STRMACRO(EGL_BIND_TO_TEXTURE_RGBA),
        STRMACRO(EGL_MIN_SWAP_INTERVAL),
        STRMACRO(EGL_MAX_SWAP_INTERVAL),
        STRMACRO(EGL_LUMINANCE_SIZE),
        STRMACRO(EGL_ALPHA_MASK_SIZE),
        STRMACRO(EGL_COLOR_BUFFER_TYPE),
        STRMACRO(EGL_RENDERABLE_TYPE),
        STRMACRO(EGL_MATCH_NATIVE_PIXMAP),
        STRMACRO(EGL_CONFORMANT),
        STRMACRO(EGL_SLOW_CONFIG),
        STRMACRO(EGL_NON_CONFORMANT_CONFIG),
        STRMACRO(EGL_TRANSPARENT_RGB),
        STRMACRO(EGL_RGB_BUFFER),
        STRMACRO(EGL_LUMINANCE_BUFFER),
        STRMACRO(EGL_FRAMEBUFFER_TARGET_ANDROID)
    };
    #undef STRMACRO

    logger->log(ml::Severity::informational, "Display EGL Configuration:", component());
    for( auto &i : egl_string_mapping)
    {
        EGLint value;
        if (eglGetConfigAttrib(disp, config, i.val, &value))
            logger->log(ml::Severity::informational,
                "    [" + i.name + "] : " + std::to_string(value), component());
    }
}

void mrl::DisplayReport::report_vsync(unsigned int output_id,
                                      graphics::Frame const& frame)
{
    std::lock_guard<decltype(vsync_event_mutex)> lk(vsync_event_mutex);
    auto prev = prev_frame.find(output_id);
    if (prev != prev_frame.end() && prev->second.msc < frame.msc)
    {
        auto const now = graphics::Frame::Timestamp::now(frame.ust.clock_id);
        auto const age_ns = now.nanoseconds - frame.ust.nanoseconds;
        auto const delta_ns = frame.ust.nanoseconds -
                              prev->second.ust.nanoseconds;

        // long long to match printf format on all architectures
        const long long msc = frame.msc,
                        age_us = age_ns.count() / 1000,
                        interval_us = delta_ns.count() /
                                      (1000*(frame.msc - prev->second.msc)),
                        hz100 = 100000000LL / interval_us;

        logger->log(component(), ml::Severity::informational,
            "vsync on %u: #%lld, %lld.%03lldms %s, interval %lld.%03lldms (%lld.%02lldHz)",
            output_id,
            msc,
            llabs(age_us/1000), llabs(age_us%1000),
            age_us>=0 ? "ago" : "from now",
            interval_us/1000, interval_us%1000,
            hz100/100, hz100%100);
    }
    prev_frame[output_id] = frame;
}
