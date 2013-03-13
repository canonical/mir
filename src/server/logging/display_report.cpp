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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/logging/display_report.h"
#include "mir/logging/logger.h"

namespace ml=mir::logging;

ml::DisplayReport::DisplayReport(const std::shared_ptr<Logger>& logger) : logger(logger)
{
}

ml::DisplayReport::~DisplayReport()
{
}

const char* ml::DisplayReport::component()
{
    static const char* s = "graphics";
    return s;
}


void ml::DisplayReport::report_successful_setup_of_native_resources()
{
    logger->log<Logger::informational>("Successfully setup native resources.", component());
}

void ml::DisplayReport::report_successful_egl_make_current_on_construction()
{
    logger->log<Logger::informational>("Successfully made egl context current on construction.", component());
}

void ml::DisplayReport::report_successful_egl_buffer_swap_on_construction()
{
    logger->log<Logger::informational>("Successfully performed egl buffer swap on construction.", component());
}

void ml::DisplayReport::report_successful_drm_mode_set_crtc_on_construction()
{
    logger->log<Logger::informational>("Successfully performed drm mode setup on construction.", component());
}

void ml::DisplayReport::report_successful_display_construction()
{
    logger->log<Logger::informational>("Successfully finished construction.", component());
}
