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

#ifndef MIR_DEFAULT_OPTIONS_H_
#define MIR_DEFAULT_OPTIONS_H_

#include "mir/options/program_option.h"

#include <memory>

namespace mir
{
class DefaultOptions
{
public:
    DefaultOptions(int argc, char const* argv[]);
    virtual ~DefaultOptions() = default;

protected:
    // add_options() allows configuration specializations to add their
    // own options. This MUST be called before the first invocation of
    // the_options() - typically during construction.
    boost::program_options::options_description_easy_init add_options();
    virtual void parse_options(boost::program_options::options_description& options_description, options::ProgramOption& options) const;
    virtual std::shared_ptr<options::Option> the_options() const;

    char const* const server_socket_opt           = "file";
    char const* const no_server_socket_opt        = "no-file";
    char const* const session_mediator_report_opt = "session-mediator-report";
    char const* const msg_processor_report_opt    = "msg-processor-report";
    char const* const display_report_opt          = "display-report";
    char const* const legacy_input_report_opt     = "legacy-input-report";
    char const* const connector_report_opt        = "connector-report";
    char const* const input_report_opt            = "input-report";
    char const* const host_socket_opt             = "host-socket";
    char const* const standalone_opt              = "standalone";

    char const* const glog                 = "glog";
    char const* const glog_stderrthreshold = "glog-stderrthreshold";
    char const* const glog_minloglevel     = "glog-minloglevel";
    char const* const glog_log_dir         = "glog-log-dir";

    bool const enable_input_default = true;

    char const* const off_opt_value = "off";
    char const* const log_opt_value = "log";
    char const* const lttng_opt_value = "lttng";

    char const* const platform_graphics_lib = "platform-graphics-lib";
    char const* const default_platform_graphics_lib = "libmirplatformgraphics.so";

private:
    int const argc;
    char const** const argv;
    std::shared_ptr<boost::program_options::options_description> const program_options;
    std::shared_ptr<options::Option> mutable options;
};
}


#endif /* MIR_DEFAULT_OPTIONS_H_ */
