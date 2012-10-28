/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_GRAPHICS_RENDERABLE_COLLECTION_H
#define MIR_GRAPHICS_RENDERABLE_COLLECTION_H

#include "mir/geometry/forward.h"

#include <memory>

namespace mir
{
namespace graphics
{
class Renderable;
}

namespace compositor
{
class RenderableFilter
{
public:
    virtual ~RenderableFilter() {}

    virtual void operator()(graphics::Renderable& renderable) = 0;

protected:
    RenderableFilter() = default;
    RenderableFilter(const RenderableFilter&) = delete;
    RenderableFilter& operator=(const RenderableFilter&) = delete;
};

class RenderableOperator
{
public:
    virtual ~RenderableOperator() {}

    virtual void operator()(graphics::Renderable& renderable) = 0;

protected:
    RenderableOperator() = default;
    RenderableOperator(const RenderableOperator&) = delete;
    RenderableOperator& operator=(const RenderableOperator&) = delete;

};

class Renderview
{
 public:
  virtual ~Renderview() {}
  
	virtual void apply(RenderableFilter& filter, RenderableOperator& operator);

 protected:
    Renderview() = default;
    
 private:
    Renderview(Renderview const&) = delete;
    Renderview& operator=(Renderview const&) = delete;
};

}
}


#endif /* MIR_GRAPHICS_RENDERABLE_COLLECTION_H */
