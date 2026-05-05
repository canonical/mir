/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_CORE_LOGGING_INTERNAL_H_
#define MIR_CORE_LOGGING_INTERNAL_H_

#include <boost/program_options.hpp>

namespace mir::logging
{
// TODO: This should probably not be in the public header; it's leaking
// implementation details
void add_logging_options(boost::program_options::options_description_easy_init options);
}

#endif // MIR_CORE_LOGGING_INTERNAL_H_
