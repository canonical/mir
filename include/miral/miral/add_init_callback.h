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

#ifndef MIRAL_ADD_INIT_CALLBACK_H
#define MIRAL_ADD_INIT_CALLBACK_H

#include <functional>

namespace mir { class Server; }

namespace miral
{
/// Add a callback to be invoked when the server has been initialized, but 
/// before it starts.
/// If multiple callbacks are added they will be invoked in the sequence added.
class AddInitCallback
{
public:
    using Callback = std::function<void()>;

    explicit AddInitCallback(Callback const& callback);
    ~AddInitCallback();

    void operator()(mir::Server& server) const;

private:
    Callback callback;
};
}


#endif //MIRAL_ADD_INIT_CALLBACK_H
