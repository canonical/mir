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

#include "user_decoration_manager_example.h"

#include <miral/decoration_manager_builder.h>

auto UserDecorationManagerExample::get_adapter()->std::shared_ptr<miral::DecorationManagerAdapter>
{
    // TODO: Copy over BasicDecoration, hook up the needed methods
    return miral::DecorationManagerBuilder::build(
               [](auto...)
               {
               },
               [](auto...)
               {
               },
               [](auto...)
               {
               },
               [](auto...)
               {
               })
        .done();
}
