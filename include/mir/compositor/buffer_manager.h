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

#ifndef BUFFER_MANAGER_H_
#define BUFFER_MANAGER_H_

#include "buffer_texture_binder.h"

namespace mir
{
namespace compositor
{

class buffer_manager : public buffer_texture_binder
{
public:

    explicit buffer_manager();

    virtual void bind_buffer_to_texture();

};

}
}


#endif /* BUFFER_MANAGER_H_ */
