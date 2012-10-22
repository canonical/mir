/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "mir/choice/program_option.h"

#include <boost/program_options/parsers.hpp>

namespace mch = mir::choice;
namespace po = boost::program_options;


mch::ProgramOption::ProgramOption()
{
}

void mch::ProgramOption::parse_arguments(
    po::options_description const& desc,
    int argc,
    char const* argv[])
{
    po::store(po::parse_command_line(argc, argv, desc), options);
    po::notify(options);
}

void mch::ProgramOption::parse_environment(
    po::options_description const& /*options*/,
    char const* /*prefix*/)
{

}

void mch::ProgramOption::parse_file(
    po::options_description const& /*options*/,
    std::string const& /*filename*/)
{

}

bool mch::ProgramOption::get(char const* /*name*/, bool default_) const
{
    return default_;
}

std::string mch::ProgramOption::get(char const* name, char const* default_) const
{
    if (options.count(name))
    {
        return options[name].as<std::string>();
    }

    return default_;
}
