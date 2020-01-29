/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_OBSERVER_REGISTRAR_H_
#define MIR_OBSERVER_REGISTRAR_H_

#include <memory>

namespace mir
{
class Executor;

/**
 * Register observers for a subsystem.
 *
 * \tparam Observer The Observer type to register
 */
template<class Observer>
class ObserverRegistrar
{
public:
    /**
     * Add an observer to the set notified of all observations
     *
     * The ObserverRegistrar does not take any ownership of \p observer, and will
     * automatically remove it when \p observer expires.
     *
     * \param [in] observer The observer to register
     */
    virtual void register_interest(std::weak_ptr<Observer> const& observer) = 0;

    /**
     * Add an observer with specified execution environment
     *
     * This is threadsafe and can be called in any context.
     *
     * The ObserverRegistrar does not take any ownership of \p observer, and will
     * automatically remove it when \p observer expires.
     *
     * All calls to \p observer methods are performed in the context of
     * \p executor.
     *
     * The \p executor should process work in a delayed fashion. Particularly,
     * executor::spawn(work) is expected to \b not run \p work in the current
     * stack. Eager execution of work may result in deadlocks if calls to the
     * observer result in calls into the ObserverRegistrar.
     *
     * \param [in] observer The observer to register
     * \param [in] executor Execution environment for calls to \p observer methods.
     *                          The caller is responsible for ensuring \p executor outlives
     *                          \p observer.
     */
    virtual void register_interest(
        std::weak_ptr<Observer> const& observer,
        Executor& executor) = 0;

    /**
     * Remove an observer from the set notified of all observations.
     *
     * This is threadsafe and can be called in any context.
     *
     * It is guaranteed that once unregister_interest() returns that no
     * other thread is in, or will receive, observations from this
     * ObserverRegistrar. If the thread calling unregister_interest()
     * is currently in an Observer call that call will complete, but
     * no further ones will be dispatched.
     *
     * \param observer [in] The observer to unregister
     */
    virtual void unregister_interest(Observer const& observer) = 0;

protected:
    ObserverRegistrar() = default;
    virtual ~ObserverRegistrar() = default;
    ObserverRegistrar(ObserverRegistrar const&) = delete;
    ObserverRegistrar& operator=(ObserverRegistrar const&) = delete;
};

}

#endif //MIR_OBSERVER_REGISTRAR_H_
