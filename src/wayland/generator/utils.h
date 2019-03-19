/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_WAYLAND_GENERATOR_WRAPPER_GENERATOR_H
#define MIR_WAYLAND_GENERATOR_WRAPPER_GENERATOR_H

#include <string>

int const all_null_types_size = 6;

// remove the path from a file path, leaving only the base name
std::string file_name_from_path(std::string const& path);

// make sure the name is not a C++ reserved word, could be expanded to get rid of invalid characters if that was needed
std::string sanitize_name(std::string const& name, bool escape_invalid = true);

// converts any string into a valid, all upper case macro name (replacing special chars with underscores)
std::string to_upper_case(std::string const& name);

// converts a snake_case string into a CamelCase string, for type names
std::string to_camel_case(std::string const& name);

#endif // MIR_WAYLAND_GENERATOR_WRAPPER_GENERATOR_H
