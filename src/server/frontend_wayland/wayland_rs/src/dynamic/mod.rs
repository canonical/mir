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

//! Protocols loaded at runtime ("dynamic" protocols).
//!
//! The protocols in `wayland-protocols/` are compiled into Mir with typed
//! bindings. This module serves protocols that Mir has never seen at build time:
//! their XML is parsed at runtime into `wayland-backend` interface descriptors,
//! and requests and events are forwarded untyped between the backend and C++
//! handlers (`DynamicGlobalHandler` / `DynamicResourceHandler`).

mod descriptors;
mod objects;

// Shared with the build script so both agree on how protocol XML is interpreted.
#[path = "../../build_script/protocol_parser.rs"]
#[allow(dead_code)]
mod protocol_parser;

pub use descriptors::load_dynamic_protocol;
pub(crate) use objects::create_dynamic_global;
pub use objects::{DynamicGlobal, DynamicResource};
