/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_SCENE_SCENE_REPORT_H_
#define MIR_SCENE_SCENE_REPORT_H_

#include <memory>

namespace mir
{
namespace scene
{
class BasicSurface;

class SceneReport
{
public:
    virtual void surface_created(BasicSurface* const surface) = 0;
    virtual void surface_added(BasicSurface* const surface) = 0;

    virtual void surface_removed(BasicSurface* const surface) = 0;
    virtual void surface_deleted(BasicSurface* const surface) = 0;

protected:
    SceneReport() = default;
    virtual ~SceneReport() = default;
    SceneReport(SceneReport const&) = delete;
    SceneReport& operator=(SceneReport const&) = delete;
};

class NullSceneReport : public SceneReport
{
public:
    virtual void surface_created(BasicSurface* const /*surface*/) override;
    virtual void surface_added(BasicSurface* const /*surface*/) override;

    virtual void surface_removed(BasicSurface* const /*surface*/) override;
    virtual void surface_deleted(BasicSurface* const /*surface*/) override;
};
}
}


#endif /* MIR_SCENE_SCENE_REPORT_H_ */
