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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_TEST_UTILS_CONFIG_
#define MIR_TEST_TEST_UTILS_CONFIG_

#include <boost/program_options/variables_map.hpp>

namespace mir
{
namespace test
{

class Config
{
public:
    Config();

    bool use_real_graphics();

private:
    boost::program_options::variables_map options;
};

}
}

#endif /* MIR_TEST_TEST_UTILS_CONFIG_ */
