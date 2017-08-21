/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_ACTIVE_OUTPUTS_H
#define MIRAL_ACTIVE_OUTPUTS_H

#include <functional>
#include <memory>
#include <vector>

namespace mir { class Server; }

namespace miral
{
class Output;

class ActiveOutputsListener
{
public:
    ActiveOutputsListener() = default;

    /// before any related calls begin
    virtual void advise_output_begin();

    /// after any related calls end
    virtual void advise_output_end();

    virtual void advise_output_create(Output const& output);
    virtual void advise_output_update(Output const& updated, Output const& original);
    virtual void advise_output_delete(Output const& output);

protected:
    virtual ~ActiveOutputsListener() = default;
    ActiveOutputsListener(ActiveOutputsListener const&) = delete;
    ActiveOutputsListener operator=(ActiveOutputsListener const&) = delete;
};

class ActiveOutputsMonitor
{
public:
    ActiveOutputsMonitor();
    ~ActiveOutputsMonitor();
    ActiveOutputsMonitor(ActiveOutputsMonitor const&);
    ActiveOutputsMonitor& operator=(ActiveOutputsMonitor const&);

    void add_listener(ActiveOutputsListener* listener);
    void delete_listener(ActiveOutputsListener* listener);

    void operator()(mir::Server& server);

    void process_outputs(std::function<void(std::vector<Output> const& outputs)> const& functor) const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_ACTIVE_OUTPUTS_H
