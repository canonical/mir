/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_OVERLAPPING_OUTPUT_GROUPING_H_
#define MIR_GRAPHICS_MESA_OVERLAPPING_OUTPUT_GROUPING_H_

#include <vector>
#include <functional>

namespace mir
{
namespace geometry
{
struct Rectangle;
}
namespace graphics
{

class DisplayConfiguration;
struct DisplayConfigurationOutput;

namespace mesa
{

class OverlappingOutputGroup
{
public:
    template <typename Iterator>
    OverlappingOutputGroup(Iterator begin, Iterator end) : outputs(begin, end) {}
    OverlappingOutputGroup(OverlappingOutputGroup const&) = default;
    OverlappingOutputGroup& operator=(OverlappingOutputGroup const&) = default;

    geometry::Rectangle bounding_rectangle() const;
    void for_each_output(std::function<void(DisplayConfigurationOutput const&)> const& f) const;

private:
    std::vector<DisplayConfigurationOutput> outputs;
};

/** Helper class that groups overlapping outputs together. */
class OverlappingOutputGrouping
{
public:
    OverlappingOutputGrouping(DisplayConfiguration const& conf);
    void for_each_group(std::function<void(OverlappingOutputGroup const& group)> const& f);

private:
    void add_output(DisplayConfigurationOutput const& conf_output);

    std::vector<OverlappingOutputGroup> groups;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_OVERLAPPING_OUTPUT_GROUPING_H_ */
