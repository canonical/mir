/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_OBSERVER_REGISTRAR_H_
#define MIR_TEST_DOUBLES_STUB_OBSERVER_REGISTRAR_H_

#include "mir/observer_registrar.h"

namespace mir
{
namespace test
{
namespace doubles
{

template<class Observer>
class StubObserverRegistrar
    : public ObserverRegistrar<Observer>
{
public:
    void register_interest(std::weak_ptr<Observer> const&) override
    {
    }

    void register_interest(
        std::weak_ptr<Observer> const&,
        mir::Executor&) override
    {
    }

    void unregister_interest(Observer const&) override
    {
    }
};

}
}
}

#endif // MIR_TEST_DOUBLES_STUB_OBSERVER_REGISTRAR_H_
