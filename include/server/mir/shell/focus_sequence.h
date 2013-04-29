/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_SHELL_FOCUS_STRATEGY_H_
#define MIR_SHELL_FOCUS_STRATEGY_H_

#include <memory>

namespace mir
{

namespace shell
{

class Session;

class FocusSequence
{
public:
    virtual ~FocusSequence() {}

    virtual std::shared_ptr<Session> successor_of(std::shared_ptr<Session> const& focused_app) const = 0;
    virtual std::shared_ptr<Session> predecessor_of(std::shared_ptr<Session> const& focused_app) const = 0;
    virtual std::shared_ptr<Session> default_focus() const = 0;

protected:
    FocusSequence() = default;
    FocusSequence(FocusSequence const&) = delete;
    FocusSequence& operator=(FocusSequence const&) = delete;
};

}
}


#endif // MIR_SHELL_FOCUS_SEQUENCE_H_
