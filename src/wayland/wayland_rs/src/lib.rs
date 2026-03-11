/*
 * Copyright © Canonical Ltd.
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

// We use include! here so that "rustfmt" does not get angry with us if
// we just mod libraries.
#[allow(dead_code, unused_imports)]
mod dispatch {
    include!("dispatch.rs");
}
#[allow(dead_code, unused_imports)]
mod protocols {
    include!("protocols.rs");
}

#[allow(dead_code, unused_imports)]
mod middleware {
    include!("middleware.rs");
}

include!("ffi.rs");
mod wayland_server;
