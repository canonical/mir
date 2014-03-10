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

#include "../null_report_factory.h"

#include "compositor_report.h"
#include "connector_report.h"
#include "message_processor_report.h"
#include "session_mediator_report.h"
#include "display_report.h"
#include "input_report.h"
#include "scene_report.h"

std::shared_ptr<mir::compositor::CompositorReport> mir::report::NullReportFactory::create_compositor_report()
{
    return std::make_shared<null::CompositorReport>();
}

std::shared_ptr<mir::graphics::DisplayReport> mir::report::NullReportFactory::create_display_report()
{
    return std::make_shared<null::DisplayReport>();
}

std::shared_ptr<mir::scene::SceneReport> mir::report::NullReportFactory::create_scene_report()
{
    return std::make_shared<null::SceneReport>();
}

std::shared_ptr<mir::frontend::ConnectorReport> mir::report::NullReportFactory::create_connector_report()
{
    return std::make_shared<null::ConnectorReport>();
}

std::shared_ptr<mir::frontend::SessionMediatorReport> mir::report::NullReportFactory::create_session_mediator_report()
{
    return std::make_shared<null::SessionMediatorReport>();
}

std::shared_ptr<mir::frontend::MessageProcessorReport> mir::report::NullReportFactory::create_message_processor_report()
{
    return std::make_shared<null::MessageProcessorReport>();
}

std::shared_ptr<mir::input::InputReport> mir::report::NullReportFactory::create_input_report()
{
    return std::make_shared<null::InputReport>();
}

std::shared_ptr<mir::compositor::CompositorReport> mir::report::null_compositor_report()
{
    return NullReportFactory{}.create_compositor_report();
}

std::shared_ptr<mir::graphics::DisplayReport> mir::report::null_display_report()
{
    return NullReportFactory{}.create_display_report();
}
std::shared_ptr<mir::scene::SceneReport> mir::report::null_scene_report()
{
    return NullReportFactory{}.create_scene_report();
}
std::shared_ptr<mir::frontend::ConnectorReport> mir::report::null_connector_report()
{
    return NullReportFactory{}.create_connector_report();
}
std::shared_ptr<mir::frontend::SessionMediatorReport> mir::report::null_session_mediator_report()
{
    return NullReportFactory{}.create_session_mediator_report();
}
std::shared_ptr<mir::frontend::MessageProcessorReport> mir::report::null_message_processor_report()
{
    return NullReportFactory{}.create_message_processor_report();
}
std::shared_ptr<mir::input::InputReport> mir::report::null_input_report()
{
    return NullReportFactory{}.create_input_report();
}
