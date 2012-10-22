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

namespace mch = mir::choice;


mch::ProgramOption::ProgramOption()
{
}

void mch::ProgramOption::parse_arguments(
    boost::program_options::options_description const& /*options*/,
    int /*argc*/,
    char const* /*argv*/[])
{
}

void mch::ProgramOption::parse_environment(
    boost::program_options::options_description const& /*options*/,
    char const* /*prefix*/)
{

}

void mch::ProgramOption::parse_file(
    boost::program_options::options_description const& /*options*/,
    std::string const& /*filename*/)
{

}

bool mch::ProgramOption::get(char const* /*name*/, bool default_) const
{
    return default_;
}

char const* mch::ProgramOption::get(char const* /*name*/, char const* default_) const
{
    return default_;
}
