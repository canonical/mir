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
 *
 * Authored By: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_EXAMPLES_CUSTOM_COMPOSITOR_H_
#define MIR_EXAMPLES_CUSTOM_COMPOSITOR_H_

namespace mir
{
class Server;

namespace examples
{
void add_custom_compositor_option_to(Server& server);
}
} // namespace mir


#endif /* MIR_EXAMPLES_CUSTOM_COMPOSITOR_H_ */
