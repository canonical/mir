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
    Self(std::string default_filter) :
        filter_(default_filter)
    {
    }

    void filter(std::string new_filter)
    {
        filter_ = new_filter;

        if(output_filter.expired())
            return;

        output_filter.lock()->set_filter(filter_);
    }

    std::string filter_;

    // output_filter is only updated during the single-threaded initialization phase of startup
    std::weak_ptr<mir::graphics::OutputFilter> output_filter;
};

miral::OutputFilter::OutputFilter()
    : self{std::make_shared<Self>("")}
{
}

miral::OutputFilter::OutputFilter(std::string default_filter)
    : self{std::make_shared<Self>(default_filter)}
{
}

miral::OutputFilter::~OutputFilter() = default;

void miral::OutputFilter::filter(std::string new_filter) const
{
    self->filter(new_filter);
}

void miral::OutputFilter::operator()(mir::Server& server) const
{
    auto const* const output_filter_opt = "output-filter";
    server.add_configuration_option(
        output_filter_opt, "Output filter to use [{none,grayscale,invert}]", self->filter_);

    server.add_init_callback(
        [&server, this, output_filter_opt]
        {
            auto const& options = server.get_options();

            self->output_filter = server.the_output_filter();
            auto const filter = options->get<std::string>(output_filter_opt);

            self->filter(filter);
        });
}
