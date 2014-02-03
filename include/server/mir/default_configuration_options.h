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

#ifndef MIR_DEFAULT_CONFIGURATION_OPTIONS_H_
#define MIR_DEFAULT_CONFIGURATION_OPTIONS_H_

#include "mir/options/program_option.h"

#include <memory>

namespace mir
{
class ConfigurationOptions
{
public:
    static char const* const server_socket_opt;
    static char const* const no_server_socket_opt;
    static char const* const enable_input_opt;
    static char const* const session_mediator_report_opt;
    static char const* const msg_processor_report_opt;
    static char const* const compositor_report_opt;
    static char const* const display_report_opt;
    static char const* const legacy_input_report_opt;
    static char const* const connector_report_opt;
    static char const* const scene_report_opt;
    static char const* const input_report_opt;
    static char const* const host_socket_opt;
    static char const* const standalone_opt;
    static char const* const frontend_threads_opt;
    static char const* const name_opt;
    static char const* const offscreen_opt;

    static char const* const glog;
    static char const* const glog_stderrthreshold;
    static char const* const glog_minloglevel;
    static char const* const glog_log_dir;
    static char const* const glog_log_dir_default;
    static int const glog_stderrthreshold_default;
    static int const glog_minloglevel_default;

    static bool const enable_input_default;

    static char const* const off_opt_value;
    static char const* const log_opt_value;
    static char const* const lttng_opt_value;

    static char const* const platform_graphics_lib;
    static char const* const default_platform_graphics_lib;

    virtual std::shared_ptr<options::Option> the_options() const = 0;

protected:

    int default_ipc_threads = 10;

    ConfigurationOptions() = default;
    virtual ~ConfigurationOptions() = default;
    ConfigurationOptions(ConfigurationOptions const&) = delete;
    ConfigurationOptions& operator=(ConfigurationOptions const&) = delete;
};

class DefaultConfigurationOptions : public ConfigurationOptions
{
public:
    DefaultConfigurationOptions(int argc, char const* argv[]);
    virtual ~DefaultConfigurationOptions() = default;

protected:
    // add_options() allows configuration specializations to add their
    // own options. This MUST be called before the first invocation of
    // the_options() - typically during construction.
    boost::program_options::options_description_easy_init add_options();
    virtual void parse_options(boost::program_options::options_description& options_description, options::ProgramOption& options) const;
    virtual std::shared_ptr<options::Option> the_options() const;

private:
    int const argc;
    char const** const argv;
    std::shared_ptr<boost::program_options::options_description> const program_options;
    std::shared_ptr<options::Option> mutable options;
};
}


#endif /* MIR_DEFAULT_CONFIGURATION_OPTIONS_H_ */
