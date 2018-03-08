/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_OPTIONS_DEFAULT_CONFIGURATION_H_
#define MIR_OPTIONS_DEFAULT_CONFIGURATION_H_

#include "mir/options/configuration.h"
#include "mir/options/program_option.h"
#include <boost/program_options/options_description.hpp>
#include <vector>

namespace mir
{
class SharedLibrary;
namespace options
{
class DefaultConfiguration : public Configuration
{
public:
    DefaultConfiguration(int argc, char const* argv[]);
    DefaultConfiguration(int argc, char const* argv[], std::string const& config_file);
    DefaultConfiguration(
        int argc, char const* argv[],
        std::function <void(int argc, char const* const* argv)> const& handler);
    DefaultConfiguration(
        int argc, char const* argv[],
        std::function <void(int argc, char const* const* argv)> const& handler,
        std::string const& config_file);
    virtual ~DefaultConfiguration() = default;

    // add_options() allows users to add their own options. This MUST be called
    // before the first invocation of the_options() - typically during initialization.
    boost::program_options::options_description_easy_init add_options();

private:
    // MUST be the first member to ensure it's destroyed last, lest we attempt to
    // call destructors in DSOs we've unloaded.
    std::vector<std::shared_ptr<SharedLibrary>> platform_libraries;

    std::string const config_file;

    void add_platform_options();
    // accessed via the base interface, when access to add_options() has been "lost"
    std::shared_ptr<options::Option> the_options() const override;

    virtual void parse_arguments(
        boost::program_options::options_description desc,
        ProgramOption& options,
        int argc,
        char const* argv[]) const;

    virtual void parse_environment(
        boost::program_options::options_description& desc,
        ProgramOption& options) const;

    virtual void parse_config_file(
        boost::program_options::options_description& desc,
        ProgramOption& options) const;

    int const argc;
    char const** const argv;
    std::function<void(int argc, char const* const* argv)> const unparsed_arguments_handler;
    std::shared_ptr<boost::program_options::options_description> const program_options;
    std::shared_ptr<Option> mutable options;
};
}
}

#endif /* MIR_OPTIONS_DEFAULT_CONFIGURATION_H_ */
