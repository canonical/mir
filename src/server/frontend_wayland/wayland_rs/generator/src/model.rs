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

//! The protocol as the C++ emitter sees it: validated, with C++ names assigned.
//!
//! Validation mirrors `register_dynamic_protocol`, so that a protocol that would be
//! rejected at runtime fails at build time instead.

use crate::protocol_parser::{
    InterfaceItem, WaylandArg, WaylandArgType, WaylandEnum, WaylandProtocol,
};
use std::collections::HashSet;

#[derive(Clone, Copy, PartialEq)]
pub enum Kind {
    Int,
    Uint,
    Fixed,
    String,
    Object,
    NewId,
    Array,
    Fd,
}

impl Kind {
    /// The `mir::wayland::DynamicArgumentKind` enumerator.
    pub fn cpp_enumerator(self) -> &'static str {
        match self {
            Kind::Int => "Int",
            Kind::Uint => "Uint",
            Kind::Fixed => "Fixed",
            Kind::String => "String",
            Kind::Object => "Object",
            Kind::NewId => "NewId",
            Kind::Array => "Array",
            Kind::Fd => "Fd",
        }
    }
}

pub struct Protocol {
    /// The protocol name from the XML.
    pub name: String,
    /// The C++ namespace (inside `mir::wayland`) holding the generated code.
    pub namespace: String,
    pub interfaces: Vec<Interface>,
}

pub struct Interface {
    pub name: String,
    pub class: String,
    pub version: u32,
    /// Whether no message of this protocol creates the interface, so it is
    /// advertised as a global.
    pub is_global: bool,
    pub requests: Vec<Message>,
    pub events: Vec<Message>,
    pub enums: Vec<Enum>,
}

pub struct Message {
    pub name: String,
    /// The C++ member function name: the request handler, or the event's
    /// `send_<name>_event`.
    pub method: String,
    pub since: u32,
    pub is_destructor: bool,
    pub args: Vec<Arg>,
}

impl Message {
    pub fn new_id(&self) -> Option<&Arg> {
        self.args.iter().find(|arg| arg.kind == Kind::NewId)
    }
}

pub struct Arg {
    pub name: String,
    /// The C++ parameter name.
    pub param: String,
    pub kind: Kind,
    pub allow_null: bool,
    pub interface: Option<String>,
}

pub struct Enum {
    pub type_name: String,
    pub entries: Vec<(String, i32)>,
}

impl Protocol {
    pub fn new(protocol: &WaylandProtocol, prefix: &str) -> Result<Self, String> {
        let namespace = cpp_identifier(&protocol.name);
        if namespace.is_empty() {
            return Err("the protocol has no name".into());
        }

        let mut interfaces = Vec::new();
        let mut names = HashSet::new();
        let mut classes = HashSet::new();
        for parsed in &protocol.interfaces {
            let interface = Interface::new(parsed, prefix)?;
            if !names.insert(interface.name.clone()) {
                return Err(format!("interface \"{}\" is defined twice", interface.name));
            }
            if !classes.insert(interface.class.clone()) {
                return Err(format!(
                    "interfaces \"{}\" and another interface both become class {}; use a different prefix",
                    interface.name, interface.class
                ));
            }
            interfaces.push(interface);
        }

        let created: HashSet<String> = interfaces
            .iter()
            .flat_map(|interface| interface.requests.iter().chain(&interface.events))
            .filter_map(|message| message.new_id()?.interface.clone())
            .collect();
        for interface in &mut interfaces {
            interface.is_global = !created.contains(&interface.name);
        }

        let result = Protocol {
            name: protocol.name.clone(),
            namespace,
            interfaces,
        };
        result.validate_events()?;
        Ok(result)
    }

    pub fn class_of(&self, interface: &str) -> Option<&str> {
        self.interfaces
            .iter()
            .find(|candidate| candidate.name == interface)
            .map(|candidate| candidate.class.as_str())
    }

    /// The typed wrapper sets the handler of an object sent with an event, so it must
    /// know the object's class.
    fn validate_events(&self) -> Result<(), String> {
        for interface in &self.interfaces {
            for event in &interface.events {
                if let Some(new_id) = event.new_id() {
                    let child = new_id.interface.as_deref().unwrap_or_default();
                    if self.class_of(child).is_none() {
                        return Err(format!(
                            "event {}.{} creates \"{child}\", which is not defined in this protocol; this is not supported",
                            interface.name, event.name
                        ));
                    }
                }
            }
        }
        Ok(())
    }
}

impl Interface {
    fn new(
        interface: &crate::protocol_parser::WaylandInterface,
        prefix: &str,
    ) -> Result<Self, String> {
        if interface.name.is_empty() {
            return Err("an interface has no name".into());
        }
        if interface.version == 0 {
            return Err(format!("interface \"{}\" has version 0", interface.name));
        }
        let stripped = interface
            .name
            .strip_prefix(prefix)
            .filter(|rest| !rest.is_empty())
            .unwrap_or(&interface.name);

        let mut result = Interface {
            name: interface.name.clone(),
            class: cpp_identifier(&snake_to_pascal(stripped)),
            version: interface.version,
            is_global: true,
            requests: Vec::new(),
            events: Vec::new(),
            enums: Vec::new(),
        };

        for item in &interface.items {
            match item {
                InterfaceItem::Request(request) => {
                    let message = result.message(
                        &request.name,
                        request.type_.as_deref(),
                        request.since,
                        &request.args,
                    )?;
                    result.requests.push(Message {
                        method: cpp_identifier(&message.name),
                        ..message
                    });
                }
                InterfaceItem::Event(event) => {
                    let message = result.message(
                        &event.name,
                        event.type_.as_deref(),
                        event.since,
                        &event.args,
                    )?;
                    result.events.push(Message {
                        method: format!("send_{}_event", cpp_identifier(&message.name)),
                        ..message
                    });
                }
                InterfaceItem::Enum(parsed) => result.enums.push(Enum::new(parsed)),
                InterfaceItem::Description(_) => {}
            }
        }

        result.validate_member_names()?;
        Ok(result)
    }

    fn message(
        &self,
        name: &str,
        type_: Option<&str>,
        since: Option<u32>,
        args: &[WaylandArg],
    ) -> Result<Message, String> {
        let context = || format!("{}.{name}", self.name);
        let since = since.unwrap_or(1);
        if since == 0 || since > self.version {
            return Err(format!(
                "{} has since=\"{since}\", outside interface version 1..={}",
                context(),
                self.version
            ));
        }

        let args = args
            .iter()
            .map(|arg| Arg::new(arg, &context))
            .collect::<Result<Vec<_>, _>>()?;
        let mut new_ids = args.iter().filter(|arg| arg.kind == Kind::NewId);
        if let Some(new_id) = new_ids.next() {
            if new_id.interface.is_none() {
                return Err(format!(
                    "{} has a new_id argument without an interface; this is not supported",
                    context()
                ));
            }
            if new_ids.next().is_some() {
                return Err(format!("{} has more than one new_id argument", context()));
            }
        }

        Ok(Message {
            name: name.to_string(),
            method: String::new(),
            since,
            is_destructor: type_ == Some("destructor"),
            args,
        })
    }

    /// Every request handler, event sender and helper is a member of the same class.
    fn validate_member_names(&self) -> Result<(), String> {
        let mut members: HashSet<String> = [
            "resource",
            "object_id",
            "version",
            "is_alive",
            "client",
            "post_error",
            "destroy_resource",
            "on_request",
            "on_constructor_request",
            "on_destroyed",
            "interface_name",
            "interface_version",
        ]
        .into_iter()
        .map(String::from)
        .collect();
        let mut types: HashSet<String> = HashSet::from(["Global".to_string()]);

        let versioned_events = self
            .events
            .iter()
            .filter(|event| event.since > 1)
            .map(version_supports);
        let methods = self
            .requests
            .iter()
            .chain(&self.events)
            .map(|message| message.method.clone())
            .chain(versioned_events);
        for method in methods {
            if !members.insert(method.clone()) {
                return Err(format!(
                    "interface \"{}\" would have two members named \"{method}\"; this is not supported",
                    self.name
                ));
            }
        }
        for enumeration in &self.enums {
            if !types.insert(enumeration.type_name.clone()) {
                return Err(format!(
                    "interface \"{}\" would have two nested types named \"{}\"; this is not supported",
                    self.name, enumeration.type_name
                ));
            }
        }
        Ok(())
    }
}

/// The name of the generated check for whether the object's version has `event`.
pub fn version_supports(event: &Message) -> String {
    format!("version_supports_{}", cpp_identifier(&event.name))
}

impl Arg {
    fn new(arg: &WaylandArg, context: &dyn Fn() -> String) -> Result<Self, String> {
        let kind = match arg.type_ {
            WaylandArgType::Int => Kind::Int,
            WaylandArgType::Uint => Kind::Uint,
            WaylandArgType::Fixed => Kind::Fixed,
            WaylandArgType::String => Kind::String,
            WaylandArgType::Object => Kind::Object,
            WaylandArgType::NewId => Kind::NewId,
            WaylandArgType::Array => Kind::Array,
            WaylandArgType::Fd => Kind::Fd,
        };
        let allow_null = arg.allow_null.unwrap_or(false);
        if allow_null && !matches!(kind, Kind::String | Kind::Object) {
            return Err(format!(
                "{} argument \"{}\" allows null, but only strings and objects can be null",
                context(),
                arg.name
            ));
        }
        let interface = arg.interface.clone().filter(|name| !name.is_empty());
        if interface.is_some() && !matches!(kind, Kind::Object | Kind::NewId) {
            return Err(format!(
                "{} argument \"{}\" names an interface, but is not an object",
                context(),
                arg.name
            ));
        }

        Ok(Arg {
            name: arg.name.clone(),
            param: cpp_identifier(&arg.name),
            kind,
            allow_null,
            interface,
        })
    }
}

impl Enum {
    fn new(enumeration: &WaylandEnum) -> Self {
        Enum {
            type_name: cpp_identifier(&snake_to_pascal(&enumeration.name)),
            entries: enumeration
                .entries
                .iter()
                .map(|entry| (cpp_identifier(&entry.name), entry.value))
                .collect(),
        }
    }
}

fn snake_to_pascal(name: &str) -> String {
    name.split('_')
        .map(|word| {
            let mut chars = word.chars();
            match chars.next() {
                None => String::new(),
                Some(first) => first.to_uppercase().collect::<String>() + chars.as_str(),
            }
        })
        .collect()
}

/// `name` as a C++ identifier: dashes become underscores, a leading digit gets a
/// leading underscore and keywords get a trailing one.
pub fn cpp_identifier(name: &str) -> String {
    const KEYWORDS: &[&str] = &[
        "alignas",
        "alignof",
        "and",
        "and_eq",
        "asm",
        "auto",
        "bitand",
        "bitor",
        "bool",
        "break",
        "case",
        "catch",
        "char",
        "char8_t",
        "char16_t",
        "char32_t",
        "class",
        "co_await",
        "co_return",
        "co_yield",
        "compl",
        "concept",
        "const",
        "const_cast",
        "consteval",
        "constexpr",
        "constinit",
        "continue",
        "decltype",
        "default",
        "delete",
        "do",
        "double",
        "dynamic_cast",
        "else",
        "enum",
        "explicit",
        "export",
        "extern",
        "false",
        "float",
        "for",
        "friend",
        "goto",
        "if",
        "inline",
        "int",
        "long",
        "mutable",
        "namespace",
        "new",
        "noexcept",
        "not",
        "not_eq",
        "nullptr",
        "operator",
        "or",
        "or_eq",
        "private",
        "protected",
        "public",
        "register",
        "reinterpret_cast",
        "requires",
        "return",
        "short",
        "signed",
        "sizeof",
        "static",
        "static_assert",
        "static_cast",
        "struct",
        "switch",
        "template",
        "this",
        "thread_local",
        "throw",
        "true",
        "try",
        "typedef",
        "typeid",
        "typename",
        "union",
        "unsigned",
        "using",
        "virtual",
        "void",
        "volatile",
        "wchar_t",
        "while",
        "xor",
        "xor_eq",
    ];

    let mut result: String = name
        .chars()
        .map(|c| if c.is_ascii_alphanumeric() { c } else { '_' })
        .collect();
    if result.starts_with(|c: char| c.is_ascii_digit()) {
        result.insert(0, '_');
    }
    if KEYWORDS.contains(&result.as_str()) {
        result.push('_');
    }
    result
}
