/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
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

#include "glog_logger.h"

#include <glog/logging.h>
#include <gflags/gflags.h>

#include <mutex>
#include <cstdlib>

namespace ml=mir::logging;

namespace
{
std::once_flag init_flag;
std::once_flag shutdown_flag;
std::once_flag shutdown_flag_gflags;

struct google_glog_guard_t
{
    google_glog_guard_t(const char* argv0)
    {
        std::call_once(init_flag, google::InitGoogleLogging, argv0);
    }

    ~google_glog_guard_t()
    {
        std::call_once(shutdown_flag, google::ShutdownGoogleLogging);
    }
};

struct google_gflag_guard_t
{
    ~google_gflag_guard_t()
    {
        std::call_once(shutdown_flag_gflags, google::ShutDownCommandLineFlags);
    }
} google_gflag_guard;
}


mir::examples::GlogLogger::GlogLogger(
    const char* argv0,
    int stderrthreshold,
    int minloglevel,
    std::string const& log_dir)
{
    FLAGS_stderrthreshold = stderrthreshold;
    FLAGS_minloglevel     = minloglevel;
    FLAGS_log_dir         = log_dir;

    static google_glog_guard_t guard(argv0);
}

void mir::examples::GlogLogger::log(ml::Severity severity, std::string const& message, std::string const& component)
{
    static int glog_level[] =
    {
        google::GLOG_FATAL,     // critical = 0,
        google::GLOG_ERROR,     // error = 1,
        google::GLOG_WARNING,   // warning = 2,
        google::GLOG_INFO,      // informational = 3,
        google::GLOG_INFO,      // debug = 4
    };

    // Since we're not collecting __FILE__ or __LINE__ this is misleading
    google::LogMessage(__FILE__, __LINE__, glog_level[static_cast<int>(severity)]).stream()
        << '[' << component << "] " << message;
}
