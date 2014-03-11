/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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


namespace mir
{
namespace options
{

class DefaultConfiguration : public Configuration
{
public:
    DefaultConfiguration(int argc, char const* argv[]);
    virtual ~DefaultConfiguration() = default;

    boost::program_options::options_description_easy_init add_options();

private:
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
    std::shared_ptr<boost::program_options::options_description> const program_options;
    std::shared_ptr<ProgramOption> mutable options;
    bool mutable parsed;
};

class GraphicsPlatformConfiguration : public Configuration
{
public:
    GraphicsPlatformConfiguration(std::shared_ptr<Configuration> const&);
    virtual ~GraphicsPlatformConfiguration() = default;
    std::shared_ptr<options::Option> the_options() const override;
    boost::program_options::options_description_easy_init add_options();

private:
    std::shared_ptr<Configuration> const default_config;
};

}
}

#endif /* MIR_OPTIONS_DEFAULT_CONFIGURATION_H_ */
