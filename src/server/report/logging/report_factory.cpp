/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "report_factory.h"

#include "compositor_report.h"
#include "connector_report.h"
#include "display_report.h"
#include "message_processor_report.h"
#include "scene_report.h"
#include "session_mediator_report.h"
#include "input_report.h"

#include "mir/default_server_configuration.h"

namespace mrl = mir::report::logging;

mrl::ReportFactory::ReportFactory(std::function<std::shared_ptr<Logger>()> && the_logger,
                                  std::function<std::shared_ptr<mir::time::Clock>()> && the_clock)
    : the_logger(std::move(the_logger)),
    the_clock(std::move(the_clock))
{
}

std::shared_ptr<mir::compositor::CompositorReport> mrl::ReportFactory::create_compositor_report()
{
    return std::make_shared<mrl::CompositorReport>(the_logger(), the_clock());
}

std::shared_ptr<mir::graphics::DisplayReport> mrl::ReportFactory::create_display_report()
{
    return std::make_shared<mrl::DisplayReport>(the_logger());
}

std::shared_ptr<mir::scene::SceneReport> mrl::ReportFactory::create_scene_report()
{
    return std::make_shared<mrl::SceneReport>(the_logger());
}

std::shared_ptr<mir::frontend::ConnectorReport> mrl::ReportFactory::create_connector_report()
{
    return std::make_shared<mrl::ConnectorReport>(the_logger());
}

std::shared_ptr<mir::frontend::SessionMediatorReport> mrl::ReportFactory::create_session_mediator_report()
{
    return std::make_shared<mrl::SessionMediatorReport>(the_logger());
}

std::shared_ptr<mir::frontend::MessageProcessorReport> mrl::ReportFactory::create_message_processor_report()
{
    return std::make_shared<mrl::MessageProcessorReport>(the_logger(), the_clock());
}

std::shared_ptr<mir::input::InputReport> mrl::ReportFactory::create_input_report()
{
    return std::make_shared<mrl::InputReport>(the_logger());
}


