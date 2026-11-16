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

//! Building `wayland-backend` interface descriptors from protocol XML at runtime.
//!
//! The backend needs a `&'static Interface` for every global and object, so
//! descriptors are leaked. Protocols are expected to be loaded once per process;
//! loading identical XML again is a no-op.
//!
//! Two kinds of interface reference appear in a message:
//! * An `object` argument's interface is only ever compared **by name** by the
//!   backend, so a name-only placeholder is sufficient (and lets a dynamic
//!   protocol refer to in-tree interfaces such as `wl_surface`).
//! * A `new_id` argument's interface becomes the created object's interface and
//!   is used to parse its requests, so it must be the full descriptor. Interfaces
//!   are therefore built children-first, and `new_id` cycles are rejected.

use super::protocol_parser::{InterfaceItem, WaylandArg, WaylandArgType, WaylandProtocol};
use std::collections::HashMap;
use std::error::Error;
use std::sync::{LazyLock, Mutex};
use wayland_server::backend::protocol::{
    AllowNull, ArgumentType, Interface, MessageDesc, ANONYMOUS_INTERFACE,
};

#[derive(Default)]
struct Registry {
    /// Protocol name -> the XML it was loaded from.
    protocols: HashMap<String, String>,
    /// Interfaces defined by loaded dynamic protocols.
    interfaces: HashMap<String, &'static Interface>,
    /// Name-only descriptors for `object` arguments naming other interfaces.
    placeholders: HashMap<String, &'static Interface>,
}

impl Registry {
    fn placeholder(&mut self, name: &str) -> &'static Interface {
        if let Some(interface) = self.interfaces.get(name) {
            return interface;
        }
        self.placeholders
            .entry(name.to_string())
            .or_insert_with(|| {
                Box::leak(Box::new(Interface {
                    name: leak_str(name),
                    version: 0,
                    requests: &[],
                    events: &[],
                    c_ptr: None,
                }))
            })
    }
}

static REGISTRY: LazyLock<Mutex<Registry>> = LazyLock::new(Default::default);

/// Look up an interface defined by a protocol loaded at runtime.
pub(super) fn find_interface(name: &str) -> Option<&'static Interface> {
    REGISTRY
        .lock()
        .unwrap_or_else(|e| e.into_inner())
        .interfaces
        .get(name)
        .copied()
}

/// Load a protocol from its XML so its interfaces can be served at runtime.
///
/// Loading the same XML again is a no-op.
///
/// # Errors
/// Returns an error, without loading anything, if the XML is invalid or the
/// protocol:
/// * defines an interface that is compiled into Mir or already loaded,
/// * has a `new_id` argument without an interface, or more than one `new_id` in
///   a message,
/// * creates (via `new_id`) an interface that is compiled into Mir or unknown,
/// * has a cycle of `new_id` references.
pub fn load_dynamic_protocol(xml: &str) -> Result<(), Box<dyn Error>> {
    let protocol: WaylandProtocol =
        quick_xml::de::from_str(xml).map_err(|e| format!("Invalid protocol XML: {e}"))?;

    let mut registry = REGISTRY.lock().unwrap_or_else(|e| e.into_inner());

    if let Some(existing) = registry.protocols.get(&protocol.name) {
        if existing == xml {
            return Ok(());
        }
        return Err(format!(
            "A different version of protocol \"{}\" is already loaded",
            protocol.name
        )
        .into());
    }

    let build_order = validate(&protocol, &registry)?;

    let mut built: HashMap<&str, &'static Interface> = HashMap::new();
    for index in build_order {
        let interface = &protocol.interfaces[index];
        let (requests, events) = messages(&protocol, index);
        let requests = leak_slice(
            requests
                .iter()
                .map(|message| build_message(message, &built, &mut registry))
                .collect(),
        );
        let events = leak_slice(
            events
                .iter()
                .map(|message| build_message(message, &built, &mut registry))
                .collect(),
        );
        let descriptor: &'static Interface = Box::leak(Box::new(Interface {
            name: leak_str(&interface.name),
            version: interface.version,
            requests,
            events,
            c_ptr: None,
        }));
        built.insert(&interface.name, descriptor);
    }

    for (name, descriptor) in built {
        registry.interfaces.insert(name.to_string(), descriptor);
    }
    registry
        .protocols
        .insert(protocol.name.clone(), xml.to_string());

    Ok(())
}

/// A request or event, independent of which it is.
struct Message<'a> {
    name: &'a str,
    is_destructor: bool,
    since: u32,
    args: &'a [WaylandArg],
}

/// The requests and events of `protocol.interfaces[index]`, in opcode order.
fn messages(protocol: &WaylandProtocol, index: usize) -> (Vec<Message<'_>>, Vec<Message<'_>>) {
    let mut requests = Vec::new();
    let mut events = Vec::new();
    for item in &protocol.interfaces[index].items {
        match item {
            InterfaceItem::Request(request) => requests.push(Message {
                name: &request.name,
                is_destructor: request.type_.as_deref() == Some("destructor"),
                since: request.since.unwrap_or(1),
                args: &request.args,
            }),
            InterfaceItem::Event(event) => events.push(Message {
                name: &event.name,
                is_destructor: event.type_.as_deref() == Some("destructor"),
                since: event.since.unwrap_or(1),
                args: &event.args,
            }),
            _ => {}
        }
    }
    (requests, events)
}

fn is_builtin(name: &str) -> bool {
    crate::BUILTIN_INTERFACE_NAMES.contains(&name)
}

/// Check that `protocol` can be loaded and return the order in which to build
/// its interfaces (every interface after the interfaces it creates via `new_id`).
fn validate(protocol: &WaylandProtocol, registry: &Registry) -> Result<Vec<usize>, String> {
    if protocol.interfaces.is_empty() {
        return Err(format!(
            "Protocol \"{}\" defines no interfaces",
            protocol.name
        ));
    }

    let mut local: HashMap<&str, usize> = HashMap::new();
    for (index, interface) in protocol.interfaces.iter().enumerate() {
        let name = interface.name.as_str();
        if interface.version == 0 {
            return Err(format!("Interface \"{name}\" has version 0"));
        }
        if is_builtin(name) {
            return Err(format!(
                "Interface \"{name}\" is already implemented by Mir"
            ));
        }
        if registry.interfaces.contains_key(name) {
            return Err(format!(
                "Interface \"{name}\" is already defined by a loaded protocol"
            ));
        }
        if local.insert(name, index).is_some() {
            return Err(format!("Interface \"{name}\" is defined more than once"));
        }
    }

    let mut children: Vec<Vec<usize>> = vec![Vec::new(); protocol.interfaces.len()];
    for (index, interface) in protocol.interfaces.iter().enumerate() {
        let (requests, events) = messages(protocol, index);
        for message in requests.iter().chain(events.iter()) {
            let context = || format!("{}.{}", interface.name, message.name);
            let mut new_ids = message
                .args
                .iter()
                .filter(|arg| arg.type_ == WaylandArgType::NewId);
            let Some(new_id) = new_ids.next() else {
                continue;
            };
            if new_ids.next().is_some() {
                return Err(format!("{} has more than one new_id argument", context()));
            }
            let Some(child) = new_id.interface.as_deref() else {
                return Err(format!(
                    "{} has a new_id argument without an interface",
                    context()
                ));
            };
            if let Some(&child_index) = local.get(child) {
                children[index].push(child_index);
            } else if is_builtin(child) {
                return Err(format!(
                    "{} creates \"{child}\", which is implemented by Mir; this is not supported",
                    context()
                ));
            } else if !registry.interfaces.contains_key(child) {
                return Err(format!(
                    "{} creates unknown interface \"{child}\"",
                    context()
                ));
            }
        }
    }

    #[derive(Clone, Copy, PartialEq)]
    enum Mark {
        Unvisited,
        InProgress,
        Done,
    }

    fn visit(
        index: usize,
        protocol: &WaylandProtocol,
        children: &[Vec<usize>],
        marks: &mut [Mark],
        order: &mut Vec<usize>,
    ) -> Result<(), String> {
        marks[index] = Mark::InProgress;
        for &child in &children[index] {
            match marks[child] {
                Mark::InProgress => {
                    return Err(format!(
                        "Interfaces \"{}\" and \"{}\" create each other via new_id; this is not supported",
                        protocol.interfaces[index].name, protocol.interfaces[child].name
                    ))
                }
                Mark::Unvisited => visit(child, protocol, children, marks, order)?,
                Mark::Done => {}
            }
        }
        marks[index] = Mark::Done;
        order.push(index);
        Ok(())
    }

    let mut marks = vec![Mark::Unvisited; protocol.interfaces.len()];
    let mut order = Vec::with_capacity(protocol.interfaces.len());
    for index in 0..protocol.interfaces.len() {
        if marks[index] == Mark::Unvisited {
            visit(index, protocol, &children, &mut marks, &mut order)?;
        }
    }
    Ok(order)
}

/// Build the descriptor of a message that [validate] has accepted.
fn build_message(
    message: &Message,
    built: &HashMap<&str, &'static Interface>,
    registry: &mut Registry,
) -> MessageDesc {
    let mut signature = Vec::with_capacity(message.args.len());
    let mut arg_interfaces: Vec<&'static Interface> = Vec::new();
    let mut child_interface = None;

    for arg in message.args {
        let allow_null = if arg.allow_null.unwrap_or(false) {
            AllowNull::Yes
        } else {
            AllowNull::No
        };
        signature.push(match arg.type_ {
            WaylandArgType::Int => ArgumentType::Int,
            WaylandArgType::Uint => ArgumentType::Uint,
            WaylandArgType::Fixed => ArgumentType::Fixed,
            WaylandArgType::String => ArgumentType::Str(allow_null),
            WaylandArgType::Array => ArgumentType::Array,
            WaylandArgType::Fd => ArgumentType::Fd,
            WaylandArgType::Object => {
                arg_interfaces.push(match arg.interface.as_deref() {
                    Some(name) => registry.placeholder(name),
                    None => &ANONYMOUS_INTERFACE,
                });
                ArgumentType::Object(allow_null)
            }
            WaylandArgType::NewId => {
                let name = arg
                    .interface
                    .as_deref()
                    .expect("validated: new_id has an interface");
                child_interface = Some(
                    built
                        .get(name)
                        .copied()
                        .or_else(|| registry.interfaces.get(name).copied())
                        .expect("validated: new_id interface is built before its creator"),
                );
                ArgumentType::NewId
            }
        });
    }

    MessageDesc {
        name: leak_str(message.name),
        signature: leak_slice(signature),
        since: message.since,
        is_destructor: message.is_destructor,
        child_interface,
        arg_interfaces: leak_slice(arg_interfaces),
    }
}

fn leak_str(s: &str) -> &'static str {
    Box::leak(s.to_string().into_boxed_str())
}

fn leak_slice<T>(items: Vec<T>) -> &'static [T] {
    Box::leak(items.into_boxed_slice())
}
