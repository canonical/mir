/*
 * Copyright © 2013-2014 Canonical Ltd.
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

#include "mir/options/configuration.h"


namespace mir
{
class DefaultConfigurationOptions : public options::Configuration
{
public:
    DefaultConfigurationOptions(int argc, char const* argv[]);
    virtual ~DefaultConfigurationOptions() = default;

    // add_options() allows users to add their own options. This MUST be called
    // before the first invocation of the_options() - typically during initialization.
    boost::program_options::options_description_easy_init add_options();

private:
    // accessed via the base interface, when access to add_options() has been "lost"
    std::shared_ptr<options::Option> the_options() const override;

    void parse_options(boost::program_options::options_description& options_description, options::ProgramOption& options) const;

    int const argc;
    char const** const argv;
    std::shared_ptr<boost::program_options::options_description> const program_options;
    std::shared_ptr<options::Option> mutable options;
};
}

#endif /* MIR_DEFAULT_CONFIGURATION_OPTIONS_H_ */
