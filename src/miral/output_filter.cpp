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

#include "miral/output_filter.h"

#include "mir/options/option.h"
#include "mir/server.h"
#include "mir/graphics/output_filter.h"

#include <string>

struct miral::OutputFilter::Self
{
    Self(MirOutputFilter default_filter) :
        filter_(default_filter)
    {
    }

    void filter(MirOutputFilter new_filter)
    {
        filter_ = new_filter;

        if(output_filter.expired())
            return;

        output_filter.lock()->filter(filter_);
    }

    MirOutputFilter filter_;

    // output_filter is only updated during the single-threaded initialization phase of startup
    std::weak_ptr<mir::graphics::OutputFilter> output_filter;
};

miral::OutputFilter::OutputFilter()
    : self{std::make_shared<Self>(mir_output_filter_none)}
{
}

miral::OutputFilter::OutputFilter(MirOutputFilter default_filter)
    : self{std::make_shared<Self>(default_filter)}
{
}

miral::OutputFilter::~OutputFilter() = default;

void miral::OutputFilter::filter(MirOutputFilter new_filter) const
{
    self->filter(new_filter);
}

void miral::OutputFilter::operator()(mir::Server& server) const
{
    auto const* const output_filter_opt = "output-filter";
    const char* default_filter_name;
    switch (self->filter_)
    {
    default:
    case mir_output_filter_none:
        default_filter_name = "none";
        break;
    case mir_output_filter_grayscale:
        default_filter_name = "grayscale";
        break;
    case mir_output_filter_invert:
        default_filter_name = "invert";
        break;
    }
    server.add_configuration_option(
        output_filter_opt, "Output filter to use [{none,grayscale,invert}]", default_filter_name);

    server.add_init_callback(
        [&server, this, output_filter_opt]
        {
            auto const& options = server.get_options();
            auto const filter_name = options->get<std::string>(output_filter_opt);

            MirOutputFilter filter = mir_output_filter_none;
            if (filter_name == "grayscale")
            {
                filter = mir_output_filter_grayscale;
            }
            else if (filter_name == "invert")
            {
                filter = mir_output_filter_invert;
            }
            else if (filter_name == "none")
            {
                filter = mir_output_filter_none;
            }

            self->output_filter = server.the_output_filter();

            self->filter(filter);
        });
}
