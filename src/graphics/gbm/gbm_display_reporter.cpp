/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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

#include "gbm_display_reporter.h"
#include "mir/logging/logger.h"


namespace mgg=mir::graphics::gbm;
namespace ml=mir::logging;


mgg::GBMDisplayReporter::GBMDisplayReporter(const std::shared_ptr<ml::Logger>& logger) : logger(logger)
{
}

mgg::GBMDisplayReporter::~GBMDisplayReporter()
{
}

const char* mgg::GBMDisplayReporter::component()
{
    static const char* s = "GBMDisplay";
    return s;
}


void mgg::GBMDisplayReporter::report_successful_setup_of_native_resources()
{
    logger->log<ml::Logger::informational>("Successfully setup native resources.", component());
}

void mgg::GBMDisplayReporter::report_successful_egl_make_current_on_construction()
{
    logger->log<ml::Logger::informational>("Successfully made egl context current on construction.", component());
}

void mgg::GBMDisplayReporter::report_successful_egl_buffer_swap_on_construction()
{
    logger->log<ml::Logger::informational>("Successfully performed egl buffer swap on construction.", component());
}

void mgg::GBMDisplayReporter::report_successful_drm_mode_set_crtc_on_construction()
{
    logger->log<ml::Logger::informational>("Successfully performed drm mode setup on construction.", component());
}

void mgg::GBMDisplayReporter::report_successful_display_construction()
{
    logger->log<ml::Logger::informational>("Successfully finished construction.", component());
}

void mgg::GBMDisplayReporter::report_page_flip_timeout()
{
    logger->log<ml::Logger::error>("Timeout while waiting for page-flip", component());
}
