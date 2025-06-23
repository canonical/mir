/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_INPUT_CURSOR_OBSERVER_MULTIPLEXER_H_
#define MIR_INPUT_CURSOR_OBSERVER_MULTIPLEXER_H_

#include "cursor_observer.h"

#include "mir/observer_multiplexer.h"

namespace mir
{
namespace input
{
class CursorObserverMultiplexer : public ObserverMultiplexer<CursorObserver>
{
public:
    explicit CursorObserverMultiplexer(Executor& default_executor);
    virtual ~CursorObserverMultiplexer();

    virtual void cursor_moved_to(float abs_x, float abs_y) override;
    virtual void pointer_usable() override;
    virtual void pointer_unusable() override;

protected:
    CursorObserverMultiplexer(CursorObserverMultiplexer const&) = delete;
    CursorObserverMultiplexer& operator=(CursorObserverMultiplexer const&) = delete;
};

}
}

#endif // MIR_INPUT_CURSOR_OBSERVER_MULTIPLEXER_H_
