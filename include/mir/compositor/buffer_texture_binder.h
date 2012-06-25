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

#ifndef BUFFER_TEXTURE_BINDER_H_
#define BUFFER_TEXTURE_BINDER_H_

namespace mir
{
namespace compositor
{

class buffer_texture_binder
{
public:

    virtual void bind_buffer_to_texture() = 0;

protected:
    buffer_texture_binder() = default;
    ~buffer_texture_binder() = default;
private:
    buffer_texture_binder(buffer_texture_binder const&) = delete;
    buffer_texture_binder& operator=(buffer_texture_binder const&) = delete;
};
}
}


#endif /* BUFFER_TEXTURE_BINDER_H_ */
