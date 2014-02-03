/*
 * Copyright © 2012 Canonical Ltd.
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

#ifndef MIR_SERVER_LOGGING_CREATE_REPORT_H_
#define MIR_SERVER_LOGGING_CREATE_REPORT_H_

#include "mir/cached_ptr.h"
#include "mir/default_server_configuration.h"
#include "mir/options/program_option.h"
#include "mir/abnormal_exit.h"

#include <memory>
#include <string>

namespace mir
{
namespace logging
{

template <typename Report, typename Logger, typename Lttng, typename NullReport>
auto create_report(DefaultServerConfiguration* config,
                   std::shared_ptr<options::Option> const& options,
                   char const* report_opt) -> std::shared_ptr<Report>
{
    auto opt = options->get<std::string>(report_opt);

    if (opt == ConfigurationOptions::log_opt_value)
    {
        return std::make_shared<Logger>(config->the_logger());
    }
    else if (opt == ConfigurationOptions::lttng_opt_value)
    {
        return std::make_shared<Lttng>();
    }
    else if (opt == ConfigurationOptions::off_opt_value)
    {
        return std::make_shared<NullReport>();
    }
    else
    {
        throw AbnormalExit(std::string("Invalid ") + report_opt + " option: " + opt + " (valid options are: \"" +
                           ConfigurationOptions::off_opt_value + "\" and \"" + ConfigurationOptions::log_opt_value +
                           "\" and \"" + ConfigurationOptions::lttng_opt_value + "\")");
    }
}

template <typename Report, typename Logger, typename Lttng, typename NullReport>
auto create_report_with_clock(DefaultServerConfiguration* config,
                              std::shared_ptr<options::Option> const& options,
                              char const* report_opt) -> std::shared_ptr<Report>
{
    auto opt = options->get<std::string>(report_opt);

    if (opt == ConfigurationOptions::log_opt_value)
    {
        return std::make_shared<Logger>(config->the_logger(), config->the_clock());
    }
    else if (opt == ConfigurationOptions::lttng_opt_value)
    {
        return std::make_shared<Lttng>();
    }
    else if (opt == ConfigurationOptions::off_opt_value)
    {
        return std::make_shared<NullReport>();
    }
    else
    {
        throw AbnormalExit(std::string("Invalid ") + report_opt + " option: " + opt + " (valid options are: \"" +
                           ConfigurationOptions::off_opt_value + "\" and \"" + ConfigurationOptions::log_opt_value +
                           "\" and \"" + ConfigurationOptions::lttng_opt_value + "\")");
    }
}
}
}

#endif
