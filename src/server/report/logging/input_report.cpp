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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "input_report.h"

#include "mir/report/legacy_input_report.h"
#include "mir/logging/logger.h"
#include "mir/logging/input_timestamp.h"

#include "std/MirLog.h"
#include <std/Log.h>

#include <linux/input.h>

#include <sstream>
#include <cstring>
#include <mutex>

namespace mrl = mir::report::logging;
namespace ml = mir::logging;
namespace mrli = mir::report::legacy_input;

namespace
{
char const* const component = "android-input";

class LegacyInputReport;

std::mutex mutex;
std::shared_ptr<LegacyInputReport> the_legacy_input_report;

class LegacyInputReport
{
public:
    LegacyInputReport(std::shared_ptr<ml::Logger> const& logger) :
        logger(logger)
    {
    }

    void log(int prio, char const* buffer)
    {
        switch (prio)
        {
        case ANDROID_LOG_UNKNOWN:
        case ANDROID_LOG_DEFAULT:
        case ANDROID_LOG_VERBOSE:
        case ANDROID_LOG_DEBUG:
            logger->log(ml::Severity::debug, buffer, component);
            break;

        case ANDROID_LOG_INFO:
            logger->log(ml::Severity::informational, buffer, component);
            break;

        case ANDROID_LOG_WARN:
            logger->log(ml::Severity::warning, buffer, component);
            break;

        case ANDROID_LOG_ERROR:
            logger->log(ml::Severity::error, buffer, component);
        };
    }

private:
    std::shared_ptr<ml::Logger> const logger;
};

void my_write_to_log(int prio, char const* buffer)
{
    std::unique_lock<std::mutex> lock(mutex);
    the_legacy_input_report->log(prio, buffer);
}
}


void mrli::initialize(std::shared_ptr<ml::Logger> const& logger)
{
    std::unique_lock<std::mutex> lock(mutex);
    ::the_legacy_input_report = std::make_shared<LegacyInputReport>(logger);

    mir::write_to_log = my_write_to_log;
}


mrl::InputReport::InputReport(const std::shared_ptr<ml::Logger>& logger)
    : logger(logger)
{
}

const char* mrl::InputReport::component()
{
    static const char* s = "input";
    return s;
}

namespace
{
#define PRINT_EV_ENUM(value, name) if (value == name) { return # name; }

std::string print_evdev_type(int type)
{
    std::stringstream out;
    PRINT_EV_ENUM(type, EV_SYN);
    PRINT_EV_ENUM(type, EV_KEY);
    PRINT_EV_ENUM(type, EV_REL);
    PRINT_EV_ENUM(type, EV_ABS);
    PRINT_EV_ENUM(type, EV_MSC);
    PRINT_EV_ENUM(type, EV_SW);
    PRINT_EV_ENUM(type, EV_LED);
    PRINT_EV_ENUM(type, EV_SND);
    PRINT_EV_ENUM(type, EV_REP);
    PRINT_EV_ENUM(type, EV_FF);
    PRINT_EV_ENUM(type, EV_PWR);
    PRINT_EV_ENUM(type, EV_FF);
    PRINT_EV_ENUM(type, EV_MAX);
    PRINT_EV_ENUM(type, EV_CNT);

    return std::to_string(type);
}

// Currently we only support pretty printing for ABS_ events as ABS_MT is the hardest protocol to debug
std::string print_evdev_code(int type, int code)
{
    if (type == EV_ABS)
    {
        PRINT_EV_ENUM(code, ABS_X);
        PRINT_EV_ENUM(code, ABS_Y);
        PRINT_EV_ENUM(code, ABS_Z);
        PRINT_EV_ENUM(code, ABS_RX);
        PRINT_EV_ENUM(code, ABS_RY);
        PRINT_EV_ENUM(code, ABS_RZ);
        PRINT_EV_ENUM(code, ABS_THROTTLE);
        PRINT_EV_ENUM(code, ABS_RUDDER);
        PRINT_EV_ENUM(code, ABS_WHEEL);
        PRINT_EV_ENUM(code, ABS_GAS);
        PRINT_EV_ENUM(code, ABS_BRAKE);
        PRINT_EV_ENUM(code, ABS_HAT0X);
        PRINT_EV_ENUM(code, ABS_HAT0Y);
        PRINT_EV_ENUM(code, ABS_HAT1X);
        PRINT_EV_ENUM(code, ABS_HAT1Y);
        PRINT_EV_ENUM(code, ABS_HAT2X);
        PRINT_EV_ENUM(code, ABS_HAT2Y);
        PRINT_EV_ENUM(code, ABS_HAT3X);
        PRINT_EV_ENUM(code, ABS_HAT3Y);
        PRINT_EV_ENUM(code, ABS_PRESSURE);
        PRINT_EV_ENUM(code, ABS_DISTANCE);
        PRINT_EV_ENUM(code, ABS_TILT_X);
        PRINT_EV_ENUM(code, ABS_TILT_Y);
        PRINT_EV_ENUM(code, ABS_TOOL_WIDTH);
        PRINT_EV_ENUM(code, ABS_VOLUME);
        PRINT_EV_ENUM(code, ABS_MISC);
        PRINT_EV_ENUM(code, ABS_MT_SLOT);
        PRINT_EV_ENUM(code, ABS_MT_TOUCH_MAJOR);
        PRINT_EV_ENUM(code, ABS_MT_TOUCH_MINOR);
        PRINT_EV_ENUM(code, ABS_MT_WIDTH_MAJOR);
        PRINT_EV_ENUM(code, ABS_MT_WIDTH_MINOR);
        PRINT_EV_ENUM(code, ABS_MT_ORIENTATION);
        PRINT_EV_ENUM(code, ABS_MT_POSITION_X);
        PRINT_EV_ENUM(code, ABS_MT_POSITION_Y);
        PRINT_EV_ENUM(code, ABS_MT_TOOL_TYPE);
        PRINT_EV_ENUM(code, ABS_MT_BLOB_ID);
        PRINT_EV_ENUM(code, ABS_MT_TRACKING_ID);
        PRINT_EV_ENUM(code, ABS_MT_PRESSURE);
        PRINT_EV_ENUM(code, ABS_MT_DISTANCE);
        // input.h from android doesn't have these even though modern kernels do.
        //        PRINT_EV_ENUM(code, ABS_MT_TOOL_X);
        //        PRINT_EV_ENUM(code, ABS_MT_TOOL_Y);
    }
    return std::to_string(code);
}
}

void mrl::InputReport::received_event_from_kernel(int64_t when, int type, int code, int value)
{
    std::stringstream ss;

    ss << "Received event"
       << " time=" << ml::input_timestamp(std::chrono::nanoseconds(when))
       << " type=" << print_evdev_type(type)
       << " code=" << print_evdev_code(type, code)
       << " value=" << value;

    logger->log(ml::Severity::informational, ss.str(), component());
}

void mrl::InputReport::published_key_event(int dest_fd, uint32_t seq_id, int64_t event_time)
{
    std::stringstream ss;

    ss << "Published key event"
       << " seq_id=" << seq_id
       << " time=" << ml::input_timestamp(std::chrono::nanoseconds(event_time))
       << " dest_fd=" << dest_fd;

    logger->log(ml::Severity::informational, ss.str(), component());
}

void mrl::InputReport::published_motion_event(int dest_fd, uint32_t seq_id, int64_t event_time)
{
    std::stringstream ss;

    ss << "Published motion event"
       << " seq_id=" << seq_id
       << " time=" << ml::input_timestamp(std::chrono::nanoseconds(event_time))
       << " dest_fd=" << dest_fd;

    logger->log(ml::Severity::informational, ss.str(), component());
}

void mrl::InputReport::received_event_finished_signal(int src_fd, uint32_t seq_id)
{
    std::stringstream ss;

    ss << "Received event finished"
       << " seq_id=" << seq_id
       << " src_fd=" << src_fd;

    logger->log(ml::Severity::informational, ss.str(), component());
}

void mrl::InputReport::opened_input_device(char const* device_name, char const* input_platform)
{
    std::stringstream ss;

    ss << "Input device opened "
       << " name=" << device_name
       << " platform=" << input_platform;

    logger->log(ml::Severity::informational, ss.str(), component());
}

void mrl::InputReport::failed_to_open_input_device(char const* device_name, char const* input_platform)
{
    std::stringstream ss;

    ss << "Failure opening input device "
       << " name=" << device_name
       << " platform=" << input_platform;

    logger->log(ml::Severity::informational, ss.str(), component());
}
