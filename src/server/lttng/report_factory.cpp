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
#include "input_report.h"
#include "message_processor_report.h"
#include "scene_report.h"
#include "session_mediator_report.h"

namespace mlttng = mir::lttng;

std::shared_ptr<mir::compositor::CompositorReport> mir::lttng::ReportFactory::create_compositor_report()
{
    return std::make_shared<mir::lttng::CompositorReport>();
}

std::shared_ptr<mir::graphics::DisplayReport> mir::lttng::ReportFactory::create_display_report()
{
    return std::make_shared<mir::lttng::DisplayReport>();
}

std::shared_ptr<mir::scene::SceneReport> mir::lttng::ReportFactory::create_scene_report()
{
    return std::make_shared<mir::lttng::SceneReport>();
}

std::shared_ptr<mir::frontend::ConnectorReport> mir::lttng::ReportFactory::create_connector_report()
{
    return std::make_shared<mir::lttng::ConnectorReport>();
}

std::shared_ptr<mir::frontend::SessionMediatorReport> mir::lttng::ReportFactory::create_session_mediator_report()
{
    return std::make_shared<mir::lttng::SessionMediatorReport>();
}

std::shared_ptr<mir::frontend::MessageProcessorReport> mir::lttng::ReportFactory::create_message_processor_report()
{
    return std::make_shared<mir::lttng::MessageProcessorReport>();
}

std::shared_ptr<mir::input::InputReport> mir::lttng::ReportFactory::create_input_report()
{
    return std::make_shared<mir::lttng::InputReport>();
}


