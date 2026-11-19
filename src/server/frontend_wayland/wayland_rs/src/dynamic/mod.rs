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

//! Protocols registered at runtime ("dynamic" protocols).
//!
//! The protocols in `wayland-protocols/` are compiled into Mir with typed
//! bindings. This module serves protocols that Mir has never seen at build time.
//! `mir_wayland_generator` turns their XML into C++ when the protocol's
//! implementation is built; at runtime that code registers a protocol
//! descriptor, from which `wayland-backend` interface descriptors are built.
//! Requests and events are forwarded untyped between the backend and the
//! generated C++ (`DynamicGlobalHandler` / `DynamicResourceHandler`), which
//! presents them as typed calls.

mod descriptors;
mod objects;

pub use descriptors::register_dynamic_protocol;
pub(crate) use objects::create_dynamic_global;
pub use objects::{DynamicGlobal, DynamicResource};
