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

#ifndef MIRAL_DECORATIONS_H
#define MIRAL_DECORATIONS_H


#include <memory>

namespace mir { class Server; }

namespace miral
{
/// Configures the window decoration strategy.
/// \note The strategy can only be applied to clients that are able to negotiate the decoration style with the server.
/// \remark Since MirAL 5.1
class Decorations
{
public:
    Decorations() = delete;
    Decorations(Decorations const&) = default;
    auto operator=(Decorations const&) -> Decorations& = default;

    void operator()(mir::Server&) const;

    /// Always use server side decorations regardless of the client's choice
    static auto always_ssd() -> Decorations;

    /// Always use client side decorations regardless of the client's choice
    static auto always_csd() -> Decorations;

    /// Prefer server side decorations if the client does not set a specific
    /// mode. Otherwise use the mode specified by the client.
    static auto prefer_ssd() -> Decorations;

    /// Prefer client side decorations if the client does not set a specific
    /// mode. Otherwise use the mode specified by the client.
    static auto prefer_csd() -> Decorations;

private:
    struct Self;

    Decorations(std::shared_ptr<Self> strategy);

    std::shared_ptr<Self> self;
};
}

#endif // MIRAL_DECORATIONS_H
