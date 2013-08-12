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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_SHELL_SURFACE_SOURCE_H_
#define MIR_SHELL_SURFACE_SOURCE_H_

#include "mir/shell/surface_factory.h"

#include <memory>

namespace mir
{

namespace shell
{
class Session;
class SurfaceBuilder;
class SurfaceConfigurator;

class SurfaceSource : public SurfaceFactory
{
public:
    SurfaceSource(std::shared_ptr<SurfaceBuilder> const& surface_builder,
                  std::shared_ptr<SurfaceConfigurator> const& surface_configurator);
    virtual ~SurfaceSource() {}

    std::shared_ptr<Surface> create_surface(
        Session* session,
        shell::SurfaceCreationParameters const& params,
        frontend::SurfaceId id,
        std::shared_ptr<frontend::EventSink> const& sink);

protected:
    SurfaceSource(const SurfaceSource&) = delete;
    SurfaceSource& operator=(const SurfaceSource&) = delete;

private:
    std::shared_ptr<SurfaceBuilder> const surface_builder;
    std::shared_ptr<SurfaceConfigurator> const surface_configurator;
};

}
}

#endif // MIR_SHELL_SURFACE_SOURCE_H_
