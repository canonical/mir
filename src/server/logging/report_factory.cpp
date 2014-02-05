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

#include "mir/logging/input_report.h"
#include "mir/default_server_configuration.h"

namespace ml = mir::logging;

ml::ReportFactory::ReportFactory(DefaultServerConfiguration &configuration)
    : configuration(configuration)
{
}

std::shared_ptr<mir::compositor::CompositorReport> ml::ReportFactory::create_compositor_report()
{
    return std::make_shared<ml::CompositorReport>(configuration.the_logger(), configuration.the_clock());
}

std::shared_ptr<mir::graphics::DisplayReport> ml::ReportFactory::create_display_report()
{
    return std::make_shared<ml::DisplayReport>(configuration.the_logger());
}

std::shared_ptr<mir::scene::SceneReport> ml::ReportFactory::create_scene_report()
{
    return std::make_shared<ml::SceneReport>(configuration.the_logger());
}

std::shared_ptr<mir::frontend::ConnectorReport> ml::ReportFactory::create_connector_report()
{
    return std::make_shared<ml::ConnectorReport>(configuration.the_logger());
}

std::shared_ptr<mir::frontend::SessionMediatorReport> ml::ReportFactory::create_session_mediator_report()
{
    return std::make_shared<ml::SessionMediatorReport>(configuration.the_logger());
}

std::shared_ptr<mir::frontend::MessageProcessorReport> ml::ReportFactory::create_message_processor_report()
{
    return std::make_shared<ml::MessageProcessorReport>(configuration.the_logger(), configuration.the_clock());
}

std::shared_ptr<mir::input::InputReport> ml::ReportFactory::create_input_report()
{
    return std::make_shared<ml::InputReport>(configuration.the_logger());
}


