/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef FRAMEBUFFER_BACKEND_H_
#define FRAMEBUFFER_BACKEND_H_

namespace mir
{
namespace graphics
{
// framebuffer_backend is the interface compositor uses onto graphics/libgl
class framebuffer_backend
{
public:
    virtual void render() = 0;

protected:
    framebuffer_backend() = default;
    ~framebuffer_backend() = default;
private:
    framebuffer_backend(framebuffer_backend const&) = delete;
    framebuffer_backend& operator=(framebuffer_backend const&) = delete;
};
}
}

#endif /* FRAMEBUFFER_BACKEND_H_ */
