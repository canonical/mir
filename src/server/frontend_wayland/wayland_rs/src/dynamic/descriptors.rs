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

//! Building `wayland-backend` interface descriptors for protocols registered at
//! runtime.
//!
//! Protocol XML is never parsed at runtime: `mir_wayland_generator` turns it into
//! C++ at build time, and that code registers a [DynamicProtocolDescriptor] here.
//!
//! The backend needs a `&'static Interface` for every global and object, so
//! descriptors are leaked. Protocols are expected to be registered once per
//! process; registering an identical descriptor again is a no-op.
//!
//! Two kinds of interface reference appear in a message:
//! * An `object` argument's interface is only ever compared **by name** by the
//!   backend, so a name-only placeholder is sufficient (and lets a dynamic
//!   protocol refer to in-tree interfaces such as `wl_surface`).
//! * A `new_id` argument's interface becomes the created object's interface and
//!   is used to parse its requests, so it must be the full descriptor. Interfaces
//!   are therefore built children-first, and `new_id` cycles are rejected.

use crate::ffi::{
    DynamicArgumentKind, DynamicInterfaceDescriptor, DynamicMessageDescriptor,
    DynamicProtocolDescriptor,
};
use std::collections::HashMap;
use std::error::Error;
use std::sync::{LazyLock, Mutex};
use wayland_server::backend::protocol::{
    AllowNull, ArgumentType, Interface, MessageDesc, ANONYMOUS_INTERFACE,
};

#[derive(Default)]
struct Registry {
    /// Protocol name -> the descriptor it was registered with.
    protocols: HashMap<String, DynamicProtocolDescriptor>,
    /// Interfaces defined by registered dynamic protocols.
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

/// Look up an interface defined by a protocol registered at runtime.
pub(super) fn find_interface(name: &str) -> Option<&'static Interface> {
    REGISTRY
        .lock()
        .unwrap_or_else(|e| e.into_inner())
        .interfaces
        .get(name)
        .copied()
}

/// Register a protocol so its interfaces can be served at runtime.
///
/// `protocol` is normally built by code that `mir_wayland_generator` generated
/// from the protocol XML. Registering an identical descriptor again is a no-op.
///
/// # Errors
/// Returns an error, without registering anything, if the protocol:
/// * has no interfaces, an interface of version 0, or an argument whose kind
///   does not allow what it specifies (e.g. a nullable `int`),
/// * defines an interface that is compiled into Mir or already registered,
/// * has a `new_id` argument without an interface, or more than one `new_id` in
///   a message,
/// * creates (via `new_id`) an interface that is compiled into Mir or unknown,
/// * has a cycle of `new_id` references,
/// * has the same name as a different, already registered, protocol.
pub fn register_dynamic_protocol(
    protocol: &DynamicProtocolDescriptor,
) -> Result<(), Box<dyn Error>> {
    let mut registry = REGISTRY.lock().unwrap_or_else(|e| e.into_inner());

    if let Some(existing) = registry.protocols.get(&protocol.name) {
        if existing == protocol {
            return Ok(());
        }
        return Err(format!(
            "A different version of protocol \"{}\" is already registered",
            protocol.name
        )
        .into());
    }

    let build_order = validate(protocol, &registry)?;

    let mut built: HashMap<&str, &'static Interface> = HashMap::new();
    for index in build_order {
        let interface = &protocol.interfaces[index];
        let requests = leak_slice(
            interface
                .requests
                .iter()
                .map(|message| build_message(message, &built, &mut registry))
                .collect(),
        );
        let events = leak_slice(
            interface
                .events
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
        .insert(protocol.name.clone(), protocol.clone());

    Ok(())
}

fn is_builtin(name: &str) -> bool {
    crate::BUILTIN_INTERFACE_NAMES.contains(&name)
}

fn messages(
    interface: &DynamicInterfaceDescriptor,
) -> impl Iterator<Item = &DynamicMessageDescriptor> {
    interface.requests.iter().chain(interface.events.iter())
}

/// Check that `protocol` can be registered and return the order in which to
/// build its interfaces (every interface after the interfaces it creates via
/// `new_id`).
fn validate(
    protocol: &DynamicProtocolDescriptor,
    registry: &Registry,
) -> Result<Vec<usize>, String> {
    if protocol.interfaces.is_empty() {
        return Err(format!(
            "Protocol \"{}\" defines no interfaces",
            protocol.name
        ));
    }

    let mut local: HashMap<&str, usize> = HashMap::new();
    for (index, interface) in protocol.interfaces.iter().enumerate() {
        let name = interface.name.as_str();
        if name.is_empty() {
            return Err(format!(
                "Protocol \"{}\" has an interface without a name",
                protocol.name
            ));
        }
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
                "Interface \"{name}\" is already defined by a registered protocol"
            ));
        }
        if local.insert(name, index).is_some() {
            return Err(format!("Interface \"{name}\" is defined more than once"));
        }
    }

    let mut children: Vec<Vec<usize>> = vec![Vec::new(); protocol.interfaces.len()];
    for (index, interface) in protocol.interfaces.iter().enumerate() {
        for message in messages(interface) {
            let context = || format!("{}.{}", interface.name, message.name);
            if message.since == 0 || message.since > interface.version {
                return Err(format!(
                    "{} has since {}, outside 1..={}",
                    context(),
                    message.since,
                    interface.version
                ));
            }
            for arg in &message.args {
                let nullable = matches!(
                    arg.kind,
                    DynamicArgumentKind::String | DynamicArgumentKind::Object
                );
                let named = matches!(
                    arg.kind,
                    DynamicArgumentKind::Object | DynamicArgumentKind::NewId
                );
                if !matches!(
                    arg.kind,
                    DynamicArgumentKind::Int
                        | DynamicArgumentKind::Uint
                        | DynamicArgumentKind::Fixed
                        | DynamicArgumentKind::String
                        | DynamicArgumentKind::Object
                        | DynamicArgumentKind::NewId
                        | DynamicArgumentKind::Array
                        | DynamicArgumentKind::Fd
                ) {
                    return Err(format!(
                        "{} argument \"{}\" has an unknown kind",
                        context(),
                        arg.name
                    ));
                }
                if arg.allow_null && !nullable {
                    return Err(format!(
                        "{} argument \"{}\" of kind {:?} cannot be nullable",
                        context(),
                        arg.name,
                        arg.kind
                    ));
                }
                if !arg.interface.is_empty() && !named {
                    return Err(format!(
                        "{} argument \"{}\" of kind {:?} cannot name an interface",
                        context(),
                        arg.name,
                        arg.kind
                    ));
                }
            }

            let mut new_ids = message
                .args
                .iter()
                .filter(|arg| arg.kind == DynamicArgumentKind::NewId);
            let Some(new_id) = new_ids.next() else {
                continue;
            };
            if new_ids.next().is_some() {
                return Err(format!("{} has more than one new_id argument", context()));
            }
            let child = new_id.interface.as_str();
            if child.is_empty() {
                return Err(format!(
                    "{} has a new_id argument without an interface",
                    context()
                ));
            }
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
        protocol: &DynamicProtocolDescriptor,
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
    message: &DynamicMessageDescriptor,
    built: &HashMap<&str, &'static Interface>,
    registry: &mut Registry,
) -> MessageDesc {
    let mut signature = Vec::with_capacity(message.args.len());
    let mut arg_interfaces: Vec<&'static Interface> = Vec::new();
    let mut child_interface = None;

    for arg in &message.args {
        let allow_null = if arg.allow_null {
            AllowNull::Yes
        } else {
            AllowNull::No
        };
        signature.push(match arg.kind {
            DynamicArgumentKind::Int => ArgumentType::Int,
            DynamicArgumentKind::Uint => ArgumentType::Uint,
            DynamicArgumentKind::Fixed => ArgumentType::Fixed,
            DynamicArgumentKind::String => ArgumentType::Str(allow_null),
            DynamicArgumentKind::Array => ArgumentType::Array,
            DynamicArgumentKind::Fd => ArgumentType::Fd,
            DynamicArgumentKind::Object => {
                arg_interfaces.push(if arg.interface.is_empty() {
                    &ANONYMOUS_INTERFACE
                } else {
                    registry.placeholder(&arg.interface)
                });
                ArgumentType::Object(allow_null)
            }
            DynamicArgumentKind::NewId => {
                child_interface = Some(
                    built
                        .get(arg.interface.as_str())
                        .copied()
                        .or_else(|| registry.interfaces.get(&arg.interface).copied())
                        .expect("validated: new_id interface is built before its creator"),
                );
                ArgumentType::NewId
            }
            _ => unreachable!("validated: argument kinds are known"),
        });
    }

    MessageDesc {
        name: leak_str(&message.name),
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
