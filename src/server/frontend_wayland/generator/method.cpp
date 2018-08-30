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

#include "method.h"

#include <libxml++/libxml++.h>

#include <iostream>

Method::Method(xmlpp::Element const& node, std::string const& class_name, bool is_global)
    : name{node.get_attribute_value("name")},
      class_name{class_name},
      is_global{is_global}
{
    for (auto const& child : node.get_children("arg"))
    {
        auto arg_node = dynamic_cast<xmlpp::Element const*>(child);
        try
        {
            arguments.emplace_back(std::ref(*arg_node));
        }
        catch (std::out_of_range const& e)
        {
            std::cerr << "failed to parse type " << arg_node->get_attribute_value("type") << std::endl;
        }
    }
}
