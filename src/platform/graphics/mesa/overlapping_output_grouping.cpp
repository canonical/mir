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

#include "overlapping_output_grouping.h"

#include "mir/graphics/display_configuration.h"
#include "mir/geometry/rectangle.h"
#include "mir/geometry/rectangles.h"

#include <unordered_set>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace geom = mir::geometry;

namespace
{

struct DCOutputHash
{
    size_t operator()(mg::DisplayConfigurationOutput const& o) const { return o.id.as_value(); };
};

struct DCOutputEqual
{
    bool operator()(mg::DisplayConfigurationOutput const& o1,
                    mg::DisplayConfigurationOutput const& o2) const
    {
        return o1.id == o2.id;
    }
};

}

/**************************
 * OverlappingOutputGroup *
 **************************/

geom::Rectangle mgm::OverlappingOutputGroup::bounding_rectangle() const
{
    geom::Rectangles rectangles;

    for (auto const& output : outputs)
    {
        geom::Rectangle const rect_output
        {
            output.top_left,
            output.modes[output.current_mode_index].size
        };
        rectangles.add(rect_output);
    }

    return rectangles.bounding_rectangle();
}

void mgm::OverlappingOutputGroup::for_each_output(
    std::function<void(DisplayConfigurationOutput const&)> const& f) const
{
    for (auto const& output : outputs)
        f(output);
}

/*****************************
 * OverlappingOutputGrouping *
 *****************************/

mgm::OverlappingOutputGrouping::OverlappingOutputGrouping(DisplayConfiguration const& conf)
{
    conf.for_each_output([&](DisplayConfigurationOutput const& conf_output)
    {
        if (conf_output.connected && conf_output.used &&
            conf_output.current_mode_index < conf_output.modes.size())
        {
            add_output(conf_output);
        }
    });

}

void mgm::OverlappingOutputGrouping::for_each_group(
    std::function<void(OverlappingOutputGroup const& group)> const& f)
{
    for (auto const& g : groups)
        f(g);
}

void mgm::OverlappingOutputGrouping::add_output(DisplayConfigurationOutput const& conf_output)
{
    std::vector<size_t> overlapping_groups;

    geom::Rectangle const rect_output
    {
        conf_output.top_left,
        conf_output.modes[conf_output.current_mode_index].size
    };

    /*
     * Find which groups the configuration overlaps with. Search in reverse
     * so that we get the indices in reverse sorted order, so we can erase
     * groups easily in the next step.
     */
    for (int i = groups.size() - 1; i >= 0; i--)
    {
        bool found_overlap{false};

        groups[i].for_each_output(
            [&](DisplayConfigurationOutput const& conf_o)
            {
                geom::Rectangle const rect
                {
                    conf_o.top_left,
                    conf_o.modes[conf_o.current_mode_index].size
                };

                if (rect.overlaps(rect_output))
                    found_overlap = true;
            });

        if (found_overlap == true)
            overlapping_groups.push_back(i);
    }

    /* Unite the groups */
    if (overlapping_groups.size() > 0)
    {
        std::unordered_set<DisplayConfigurationOutput, DCOutputHash, DCOutputEqual> new_group;

        for (auto i : overlapping_groups)
        {
            groups[i].for_each_output([&](DisplayConfigurationOutput const& conf_o)
            {
                new_group.insert(conf_o);
            });

            /*
             * Erase the processed group. We can do this safely since the group indices
             * are in reverse sorted order.
             */
            groups.erase(groups.begin() + i);
        }

        new_group.insert(conf_output);

        groups.push_back(OverlappingOutputGroup{new_group.begin(), new_group.end()});
    }
    else
    {
        std::vector<mg::DisplayConfigurationOutput> v{conf_output};
        groups.push_back(OverlappingOutputGroup{v.begin(), v.end()});
    }
}
