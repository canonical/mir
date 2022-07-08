/*
 * Copyright © 2015 Canonical Ltd.
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

#ifndef MIR_SHELL_SURFACE_READY_REGISTRAR_H_
#define MIR_SHELL_SURFACE_READY_REGISTRAR_H_

#include <functional>
#include <memory>
#include <map>
#include <mutex>

namespace mir
{
namespace scene { class Session; class Surface; }
namespace geometry { struct Size; }

namespace shell
{
/// Notifies the user when a surface is ready
class SurfaceReadyRegistrar
{
public:
    using ActivateFunction = std::function<void(std::shared_ptr<scene::Surface> const& surface)>;

    SurfaceReadyRegistrar(ActivateFunction const& surface_ready);
    ~SurfaceReadyRegistrar();
    void register_surface(std::shared_ptr<scene::Surface> const& surface);
    void unregister_surface(scene::Surface& surface);
    void clear();

private:
    struct Observer;
    ActivateFunction const surface_ready;

    std::mutex mutex;
    std::map<scene::Surface*, std::shared_ptr<Observer>> observers;
};
}
}

#endif /* MIR_SHELL_SURFACE_READY_REGISTRAR_H_ */
