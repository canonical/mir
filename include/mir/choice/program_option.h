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


#ifndef PROGRAM_OPTION_H_
#define PROGRAM_OPTION_H_

#include "option.h"

#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>

namespace mir
{
namespace choice
{
class ProgramOption : public Option
{
public:
    ProgramOption();

    void parse_arguments(
        boost::program_options::options_description const& options,
        int argc,
        char const* argv[]);

    void parse_environment(
        boost::program_options::options_description const& options,
        char const* prefix);

    void parse_file(
        boost::program_options::options_description const& options,
        std::string const& filename);

    bool get(char const* name, bool default_) const;
    std::string get(char const*, char const* default_) const;

private:
    boost::program_options::variables_map options;
};
}
}


#endif /* PROGRAM_OPTION_H_ */
