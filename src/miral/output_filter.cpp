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

#include <atomic>
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

    std::atomic<MirOutputFilter> filter_;

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
    server.add_init_callback(
        [&server, self=self]
        {
            auto output_filter = server.the_output_filter();
            output_filter->filter(self->filter_);
            self->output_filter = output_filter;
        });
}
