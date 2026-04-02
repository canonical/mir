/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_LIVE_CONFIG_PARSING_STORE_H
#define MIRAL_LIVE_CONFIG_PARSING_STORE_H

#include "live_config.h"

#include <filesystem>

namespace miral::live_config
{
class ParsingStore : public Store
{
public:
    /// Loads the configuration from the given stream. The path is used for
    /// error reporting. Consequent calls to this method will add to the
    /// existing configuration, with later values assigned to the same key
    /// taking precedence over earlier ones for scalars, and appending onto
    /// arrays.
    ///
    /// Arrays can be cleared by assigning an empty value to the key, e.g.
    /// `my_array_key=`.
    virtual void load_file(std::istream& istream, std::filesystem::path const& path) = 0;
};
}

#endif
