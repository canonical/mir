/*
 * Copyright © 2014,2021 Canonical Ltd.
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

#ifndef MIRAL_DISPLAY_CONFIGURATION_OPTIONS_H_
#define MIRAL_DISPLAY_CONFIGURATION_OPTIONS_H_

namespace mir { class Server; }

namespace miral
{
/** Enable the display configuration options.
 *   * display-config  {clone,sidebyside,single,static=<filename>}
 *   * translucent     {on,off}
 * \remark Since MirAL 2.4
 */
void display_configuration_options(mir::Server& server);
}


#endif /* MIRAL_DISPLAY_CONFIGURATION_OPTIONS_H_ */
