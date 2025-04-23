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

#ifndef MIRAL_MAGNIFICATION_H
#define MIRAL_MAGNIFICATION_H

#include <memory>
#include <functional>

#include "mir/geometry/displacement.h"

namespace mir
{
class Server;
}

/// Enables configuring the magnification accessibility feature.
/// \remark Since MirAL 5.3
namespace miral
{
class Magnification
{
public:
    explicit Magnification(bool enabled_by_default);
    void operator()(mir::Server& server) const;
    bool enabled(bool enabled) const;
    void magnification(float magnification) const;
    float magnification() const;
    void size(mir::geometry::Size const& size) const;
    mir::geometry::Size size() const;

    Magnification& on_enabled(std::function<void()>&& callback);
    Magnification& on_disabled(std::function<void()>&& callback);

private:
    class Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_MAGNIFICATION_H
