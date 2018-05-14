/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/default_server_configuration.h"
#include "mir/options/configuration.h"

#include "reports.h"
#include "lttng_report_factory.h"
#include "logging_report_factory.h"
#include "null_report_factory.h"

#include "mir/abnormal_exit.h"

namespace mg = mir::graphics;
namespace mf = mir::frontend;
namespace mc = mir::compositor;
namespace mi = mir::input;
namespace ms = mir::scene;

std::unique_ptr<mir::report::ReportFactory> mir::DefaultServerConfiguration::report_factory(char const* report_opt)
{
    auto opt = the_options()->get<std::string>(report_opt);

    if (opt == options::log_opt_value)
    {
        return std::make_unique<report::LoggingReportFactory>(the_logger(), the_clock());
    }
    else if (opt == options::lttng_opt_value)
    {
        return std::make_unique<report::LttngReportFactory>();
    }
    else if (opt == options::off_opt_value)
    {
        return std::make_unique<report::NullReportFactory>();
    }
    else
    {
        throw AbnormalExit(std::string("Invalid ") + report_opt + " option: " + opt + " (valid options are: \"" +
            options::off_opt_value + "\" and \"" + options::log_opt_value +
                           "\" and \"" + options::lttng_opt_value + "\")");
    }
}

std::shared_ptr<void> mir::DefaultServerConfiguration::default_reports()
{
    return std::make_unique<report::Reports>(*this, *the_options());
}

auto mir::DefaultServerConfiguration::the_compositor_report() -> std::shared_ptr<mc::CompositorReport>
{
    return compositor_report(
        [this]()->std::shared_ptr<mc::CompositorReport>
        {
            return report_factory(options::compositor_report_opt)->create_compositor_report();
        });
}

auto mir::DefaultServerConfiguration::the_connector_report() -> std::shared_ptr<mf::ConnectorReport>
{
    return connector_report(
        [this]()->std::shared_ptr<mf::ConnectorReport>
        {
            return report_factory(options::connector_report_opt)->create_connector_report();
        });
}

auto mir::DefaultServerConfiguration::the_message_processor_report() -> std::shared_ptr<mf::MessageProcessorReport>
{
    return message_processor_report(
        [this]()->std::shared_ptr<mf::MessageProcessorReport>
        {
            return report_factory(options::msg_processor_report_opt)->create_message_processor_report();
        });
}

auto mir::DefaultServerConfiguration::the_display_report() -> std::shared_ptr<mg::DisplayReport>
{
    return display_report(
        [this]()->std::shared_ptr<mg::DisplayReport>
        {
            return report_factory(options::display_report_opt)->create_display_report();
        });
}

auto mir::DefaultServerConfiguration::the_input_report() -> std::shared_ptr<mi::InputReport>
{
    return input_report(
        [this]()->std::shared_ptr<mi::InputReport>
        {
            return report_factory(options::input_report_opt)->create_input_report();
        });
}

auto mir::DefaultServerConfiguration::the_scene_report() -> std::shared_ptr<ms::SceneReport>
{
    return scene_report(
        [this]()->std::shared_ptr<ms::SceneReport>
        {
            return report_factory(options::scene_report_opt)->create_scene_report();
        });
}

auto mir::DefaultServerConfiguration::the_shared_library_prober_report() -> std::shared_ptr<SharedLibraryProberReport>
{
    return shared_library_prober_report(
        [this]()->std::shared_ptr<SharedLibraryProberReport>
        {
            return report_factory(options::shared_library_prober_report_opt)->create_shared_library_prober_report();
        });
}

auto mir::DefaultServerConfiguration::the_shell_report() -> std::shared_ptr<shell::ShellReport>
{
    return shell_report(
        [this]()->std::shared_ptr<shell::ShellReport>
        {
            return report_factory(options::shell_report_opt)->create_shell_report();
        });
}

