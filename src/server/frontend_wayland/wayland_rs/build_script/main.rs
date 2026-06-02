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

mod cpp_builder;
mod dispatch_generator;
mod ffi_generation;
mod helpers;
mod protocol_generator;
mod protocol_middleware_generation;
mod protocol_parser;
mod wayland_server_generation;

use cpp_builder::{
    sanitize_identifier, CppArg, CppBuilder, CppClass, CppEnum, CppEnumOption, CppMethod,
    CppNamespace, CppType,
};
use ffi_generation::generate_ffi;
use helpers::*;
use protocol_middleware_generation::generate_wayland_interface_middleware;
use protocol_parser::{
    parse_protocols, InterfaceItem, WaylandArg, WaylandArgType, WaylandEnum, WaylandEvent,
    WaylandInterface, WaylandProtocol, WaylandRequest,
};
use std::{env, path::Path};

use crate::dispatch_generator::generate_dispatch_rs;
use crate::protocol_generator::generate_protocols_rs;
use crate::wayland_server_generation::generate_wayland_server_generated_rs;

fn main() {
    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let include_path = Path::new(&manifest_dir).join(".");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/wayland_server_core.rs");
    println!("cargo:rerun-if-changed=build_script/ffi_generation.rs");
    println!("cargo:rerun-if-changed=build_script/dispatch_generator.rs");
    println!("cargo:rerun-if-changed=build_script/protocol_generator.rs");
    println!("cargo:rerun-if-changed=build_script/protocol_middleware_generation.rs");
    println!("cargo:rerun-if-changed=build_script/wayland_server_generation.rs");

    // First, parse the protocol XML files.
    let protocols: Vec<WaylandProtocol> = parse_protocols();

    // Next, generate the protocols.rs file.
    write_protocols_rs(&protocols);

    // Next, generate the dispatch and global dispatch methods.
    write_dispatch_rs(&protocols);

    // Next, generate the protocol middleware classes.
    write_protocol_middleware(&protocols);

    // Next, generate C++ abstract classes for each interface
    // as well as the FFI code.
    write_cpp_protocol_implementations(&protocols);

    // Next, write the generated side of the WaylandServer module.
    write_wayland_server_generated(&protocols);

    // Finally, declare the bridges.
    // This must happen last because `src/ffi.rs` is built by this script and
    // it may not exist before the build is run.
    cxx_build::bridges(vec!["src/ffi.rs"])
        .include(&include_path)
        .compile("wayland_rs");
}

fn write_protocols_rs(protocols: &Vec<WaylandProtocol>) {
    let tokens = generate_protocols_rs(protocols);
    write_generated_rust_file(tokens, "protocols.rs");
}

fn write_dispatch_rs(protocols: &Vec<WaylandProtocol>) {
    let tokens = generate_dispatch_rs(protocols);
    write_generated_rust_file(tokens, "dispatch.rs");
}

fn create_global_factory(protocols: &Vec<WaylandProtocol>) -> CppBuilder {
    let mut builder: CppBuilder = CppBuilder::new("MIR_WAYLANDRS_GLOBALS", "global_factory");
    builder.add_header_include("<memory>");
    builder.add_header_include("<rust/cxx.h>");
    let mut namespace = CppNamespace::new(vec!["mir", "wayland_rs"]);
    let mut class = CppClass::new("GlobalFactory");
    namespace.add_forward_declaration_class("WaylandClient");
    protocols.iter().for_each(|protocol| {
        protocol
            .interfaces
            .iter()
            .filter(|interface| interface.is_global)
            .filter(|interface| interface.name != "wl_display" && interface.name != "wl_registry")
            .for_each(|global_interface| {
                let class_name = format_wayland_interface_to_cpp_class(&global_interface.name);
                namespace.add_forward_declaration_class(&class_name);

                let mut method = CppMethod::new(
                    format!("create_{}", global_interface.name),
                    Some(CppType::Object(class_name)),
                    true,
                    false,
                    true,
                    true,
                );
                method.add_arg(CppArg::new(
                    CppType::Box("WaylandClient".to_string()),
                    "client",
                    false,
                ));
                class.add_method(method);
            })
    });

    let mut can_view_method =
        CppMethod::new("can_view", Some(CppType::Bool), true, false, true, true);
    can_view_method.add_arg(CppArg::new(CppType::Str, "interface_name", false));
    can_view_method.add_arg(CppArg::new(
        CppType::Box("WaylandClientId".to_string()),
        "client_id",
        false,
    ));
    class.add_method(can_view_method);

    namespace.add_class(class);
    namespace.add_forward_declaration_class("WaylandClientId");
    builder.add_namespace(namespace);
    builder
}

fn create_wayland_server_notification_handler() -> CppBuilder {
    let mut builder: CppBuilder = CppBuilder::new(
        "MIR_WAYLANDRS_WAYLAND_SERVER_NOTIFICATION_HANDLER",
        "wayland_server_notification_handler",
    );
    builder.add_header_include("<memory>");
    builder.add_header_include("<rust/cxx.h>");

    builder.add_cpp_include("\"wayland_rs/src/ffi.rs.h\"");
    let mut namespace = CppNamespace::new(vec!["mir", "wayland_rs"]);
    namespace.add_forward_declaration_class("WaylandClient");
    namespace.add_forward_declaration_class("WaylandClientId");
    let mut class = CppClass::new("WaylandServerNotificationHandler");

    let mut client_added_method = CppMethod::new("client_added", None, true, false, true, true);
    client_added_method.add_arg(CppArg::new(
        CppType::Box("WaylandClient".to_string()),
        "wayland_client",
        false,
    ));
    class.add_method(client_added_method);

    let mut client_removed_method = CppMethod::new("client_removed", None, true, false, true, true);
    client_removed_method.add_arg(CppArg::new(
        CppType::Box("WaylandClientId".to_string()),
        "id",
        false,
    ));
    class.add_method(client_removed_method);

    namespace.add_class(class);
    builder.add_namespace(namespace);
    builder
}

fn write_protocol_middleware(protocols: &Vec<WaylandProtocol>) {
    let middleware = generate_wayland_interface_middleware(protocols);
    write_generated_rust_file(middleware, "middleware.rs");
}

/// Write a header file for each protocol containing abstract classes per-interface.
fn write_cpp_protocol_implementations(protocols: &Vec<WaylandProtocol>) {
    // First, create the global factory.
    let global_builder = create_global_factory(protocols);

    // Then, create the WaylandServerNotificationHandler.
    let wayland_server_notification_handler_builder = create_wayland_server_notification_handler();

    // Generate ffi_fwd.h first so that protocol headers can include it without
    // creating a circular dependency with the CXX-generated ffi.rs.h.
    let ffi_fwd_builder = create_ffi_fwd_builder(protocols);
    write_cpp_header(&ffi_fwd_builder);

    let mut builders: Vec<CppBuilder> = protocols
        .iter()
        .map(|protocol| create_cpp_builder(protocol))
        .collect();
    builders.push(global_builder);
    builders.push(wayland_server_notification_handler_builder);

    // Write the protocol headers
    for builder in &builders {
        write_cpp_header(&builder);
        write_cpp_source(&builder);
    }

    let ffi = generate_ffi(&protocols, &builders);
    write_generated_rust_file(ffi, "ffi.rs");
}

/// Create a lightweight header containing forward declarations for every Rust opaque
/// type used as `rust::Box<T>` in the protocol headers.
///
/// Protocol headers include this header instead of the CXX-generated ffi.rs.h,
/// which itself includes all protocol headers — inclusion of ffi.rs.h from a
/// protocol header would create a circular dependency.
///
/// Plain `struct Foo;` forward declarations are sufficient because rust::Box<T>
/// only stores a T* internally and does not require T to be a complete type at
/// the point of a virtual function declaration.
fn create_ffi_fwd_builder(protocols: &Vec<WaylandProtocol>) -> CppBuilder {
    let mut builder = CppBuilder::new("MIR_WAYLANDRS_FFI_FWD", "ffi_fwd");
    // <rust/cxx.h> is included here so that protocol headers pulling in ffi_fwd.h
    // have access to rust::Box, rust::String, etc. without a direct dependency on ffi.rs.h.
    builder.add_header_include("<rust/cxx.h>");
    let mut namespace = CppNamespace::new(vec!["mir", "wayland_rs"]);

    // WaylandServer is declared in ffi.rs but used nowhere in the protocol headers;
    // include it for completeness so all Rust types are forward-declared.
    namespace.add_forward_declaration_class("WaylandServer");
    // WaylandClient is used in the protected constructor of every generated XxxImpl.
    namespace.add_forward_declaration_class("WaylandClient");

    for protocol in protocols {
        for interface in &protocol.interfaces {
            if interface.name == "wl_registry" || interface.name == "wl_display" {
                continue;
            }
            namespace.add_forward_declaration_struct(
                &format_wayland_interface_to_rust_extension_struct(&interface.name),
            );
        }
    }

    builder.add_namespace(namespace);
    builder
}

fn create_cpp_builder(protocol: &WaylandProtocol) -> CppBuilder {
    let guard = format!("MIR_WAYLANDRS_{}", protocol.name.to_uppercase());
    let mut builder = CppBuilder::new(guard, protocol.name.as_str());
    let mut namespace = CppNamespace::new(vec!["mir", "wayland_rs"]);

    let classes = protocol
        .interfaces
        .iter()
        .filter(|interface| interface.name != "wl_registry" && interface.name != "wl_display")
        .map(|interface| wayland_interface_to_cpp_class(interface));

    for class in classes {
        namespace.add_class(class);
    }
    for interface in &protocol.dependencies {
        let class_name = format_wayland_interface_to_cpp_class(interface);
        namespace.add_forward_declaration_class(&class_name);
    }
    builder.add_namespace(namespace);
    // Use ffi_fwd.h (generated alongside the protocol headers) instead of the
    // CXX-generated ffi.rs.h to avoid a circular include dependency:
    //   ffi.rs.h  →  include/*.h  →  ffi.rs.h
    builder.add_header_include("\"lifetime_tracker.h\"");
    builder.add_header_include("\"ffi_fwd.h\"");
    builder.add_header_include("\"weak.h\"");
    builder.add_header_include("<memory>");
    builder.add_header_include("<optional>");
    builder.add_header_include("<cstdint>");
    builder.add_header_include("<rust/cxx.h>");

    builder.add_cpp_include("\"wayland_rs/src/ffi.rs.h\"");

    builder
}

fn write_cpp_header(builder: &CppBuilder) {
    let filename = format!("{}.h", builder.filename);
    write_generated_cpp_file(
        &builder.to_cpp_header(),
        "wayland_rs_cpp/include",
        filename.as_str(),
    );
}

fn write_cpp_source(builder: &CppBuilder) {
    let header_filename = format!("{}.h", builder.filename);

    let filename = format!("{}.cpp", builder.filename);
    write_generated_cpp_file(
        &builder.to_cpp_source(&header_filename),
        "wayland_rs_cpp/src",
        filename.as_str(),
    );
}

fn wayland_interface_to_cpp_class(interface: &WaylandInterface) -> CppClass {
    let class_name = format_wayland_interface_to_cpp_class(&interface.name);
    let mut class = CppClass::new(class_name);
    class.set_superclass("LifetimeTracker");
    let methods = interface
        .items
        .iter()
        .filter_map(|item| {
            if let protocol_parser::InterfaceItem::Request(request) = item {
                Some(wayland_request_to_cpp_method(request))
            } else if let protocol_parser::InterfaceItem::Event(event) = item {
                Some(vec![wayland_event_to_cpp_method(event)])
            } else {
                None
            }
        })
        .flatten();

    for enum_ in wayland_interface_to_enums(interface) {
        class.add_enum(enum_);
    }

    // Add the method that associates the boxed rust interface with the C++ class.
    let mut associate_method = CppMethod::new("associate", None, true, false, true, true);
    associate_method.add_arg(CppArg::new(
        CppType::Box(snake_to_pascal(
            &format_wayland_interface_to_rust_extension_struct(&interface.name),
        )),
        "instance",
        false,
    ));
    associate_method.add_arg(CppArg::new(CppType::CppU32, "object_id", false));
    associate_method.set_body("object_id_ = object_id;\ninstance_ = std::move(instance);");
    class.add_method(associate_method);
    class.add_private_member(CppArg::new(
        CppType::Box(snake_to_pascal(
            &format_wayland_interface_to_rust_extension_struct(&interface.name),
        )),
        "instance_",
        true,
    ));

    // Add a private member for the object ID of the associated resource.
    class.add_private_member(CppArg::new(CppType::CppU32, "object_id_", false));

    // Add a public accessor for the object ID so that C++ implementations
    // can pass it to ProtocolError.
    let mut object_id_method = CppMethod::new(
        "object_id",
        Some(CppType::CppU32),
        false,
        false,
        false,
        false,
    );
    object_id_method.set_body("return object_id_;");
    class.add_method(object_id_method);

    // Add the "get_box" method that will return the boxes rust interface. This is used
    // when sending a Box from C++ to Rust.
    let mut get_box_method = CppMethod::new(
        "get_box",
        Some(CppType::Box(snake_to_pascal(
            &format_wayland_interface_to_rust_extension_struct(&interface.name),
        ))),
        false,
        false,
        true,
        true,
    );
    get_box_method.set_body(
        "if (!instance_) { throw \"get_box() called before associate()\"; }\nreturn instance_.value();",
    );
    class.add_method(get_box_method);

    // Add a post_error method that posts a protocol error to the client for this resource.
    // This follows the same pattern as event-sending methods, routing through the Rust
    // middleware Ext type to call wayland-server's DisplayHandle::post_error().
    let mut post_error_method = CppMethod::new("post_error", None, false, false, false, true);
    post_error_method.add_arg(CppArg::new(CppType::CppU32, "code", false));
    post_error_method.add_arg(CppArg::new(CppType::String, "message", false));
    post_error_method
        .set_body("assert(instance_.has_value());\ninstance_.value()->post_error(code, message);");
    class.add_method(post_error_method);

    // Add a protected constructor and member so that subclasses know which client
    // they are serving and can interact with it as they please.
    class.add_protected_constructor_arg(CppArg::new(
        CppType::Box("WaylandClient".to_string()),
        "client",
        false,
    ));
    class
        .add_public_member(CppArg::new(
            CppType::Box("WaylandClient".to_string()),
            "client",
            false,
        ))
        .set_const();
    for method in methods {
        class.add_method(method);
    }

    class
}

fn wayland_interface_to_enums(interface: &WaylandInterface) -> impl Iterator<Item = CppEnum> + '_ {
    interface.items.iter().filter_map(|item| {
        if let protocol_parser::InterfaceItem::Enum(enum_) = item {
            Some(wayland_enum_to_cpp_enum(enum_))
        } else {
            None
        }
    })
}

fn wayland_enum_to_cpp_enum(enum_: &WaylandEnum) -> CppEnum {
    let mut result = CppEnum::new(snake_to_pascal(enum_.name.as_str()));

    for option in &enum_.entries {
        result.add_option(CppEnumOption::new(
            option.name.as_str(),
            option.value as u32,
        ));
    }

    result
}

fn wayland_arg_to_cpp_arg(arg: &WaylandArg) -> CppArg {
    let type_ = match arg.type_ {
        WaylandArgType::Int => CppType::CppI32,
        WaylandArgType::Uint => CppType::CppU32,
        WaylandArgType::Fixed => CppType::CppF64,
        WaylandArgType::String => CppType::String,
        WaylandArgType::Object => CppType::Object(format_wayland_interface_to_cpp_class(
            arg.interface.as_ref().expect("Object is missing interface"),
        )),
        WaylandArgType::Array => CppType::Array,
        WaylandArgType::Fd => CppType::Fd,
        WaylandArgType::NewId => CppType::Box(format_wayland_interface_to_rust_extension_struct(
            arg.interface.as_ref().expect("NewId is missing interface"),
        )),
    };

    CppArg::new(type_, arg.name.as_str(), arg.allow_null.unwrap_or(false))
}

fn wayland_request_to_cpp_method(method: &WaylandRequest) -> Vec<CppMethod> {
    // Methods will have a return value in C++ if they are creating a new Wayland object.
    // Otherwise they always return void.
    let retval = match method
        .args
        .iter()
        .find(|arg| arg.type_ == WaylandArgType::NewId)
    {
        Some(new_id_arg) => {
            let name = format_wayland_interface_to_cpp_class(
                new_id_arg
                    .interface
                    .as_ref()
                    .expect("NewId is missing interface"),
            );
            Some(CppType::Object(name))
        }
        None => None,
    };

    let args = method
        .args
        .iter()
        .filter(|arg| arg.type_ != WaylandArgType::NewId)
        .map(wayland_arg_to_cpp_arg);

    // If a C++ argument is being sent from Rust to C++ as a shared_ptr, we do NOT want
    // to encourage the protocol implementer to hold a shared reference to the object, as
    // that muddies up the lifetimes significantly. Only the Rust Wayland frontend should
    // have access to the lifetime. However, it is important for the protocol business logic
    // to be able to interact with other protocol implementations. Hence, we wrap the shared_ptr
    // in our own "Weak" construct.
    //
    // If this is the case, we need to do the following:
    // 1. Generate a different virtual method that is NOT called from Rust which will handle
    //    the logic using the weak value.
    // 2. Make the original handler NOT be virtual, as the overridable logic will be delegated
    //    to the new method from #1.
    let wayland_weak_transformers: Vec<String> = args
        .clone()
        .flat_map(|arg| match arg.cpp_type() {
            CppType::Object(type_name) => Some(format!(
                "auto const wrapped_{name} = wayland_rs::Weak<{type_name}>({name});",
                name = arg.name(),
                type_name = type_name
            )),
            _ => None,
        })
        .collect();

    let delegation_args: Vec<String> = args
        .clone()
        .flat_map(|arg| {
            let call_arg = match arg.cpp_type() {
                CppType::Object(_) => format!("wrapped_{}", arg.name()),
                _ => arg.name().to_string(),
            };
            if let Some(has_name) = arg.has_name() {
                vec![call_arg, has_name]
            } else {
                vec![call_arg]
            }
        })
        .collect();

    let has_retval = retval.is_some();
    let mut cpp_method = CppMethod::new(
        method.name.as_str(),
        retval,
        wayland_weak_transformers.is_empty(),
        true,
        true,
        true,
    );
    for arg in args {
        cpp_method.add_arg(arg);
    }

    if let Some(type_) = &method.type_ {
        if type_ == "destructor" {
            assert!(wayland_weak_transformers.is_empty());
            cpp_method.set_body("");
        }
    }

    if !wayland_weak_transformers.is_empty() {
        // Build the body: wrap shared_ptrs in Weak, then delegate to the virtual overload
        let return_prefix = if has_retval { "return " } else { "" };
        let delegation_call = format!(
            "{}{}({});",
            return_prefix,
            sanitize_identifier(&method.name),
            delegation_args.join(", ")
        );
        let mut body_lines = wayland_weak_transformers;
        body_lines.push(delegation_call);
        cpp_method.set_body(body_lines.join("\n"));

        // Generate the virtual method that protocol implementers override.
        // This method uses Weak args instead of shared_ptr args.
        let virtual_retval = method
            .args
            .iter()
            .find(|arg| arg.type_ == WaylandArgType::NewId)
            .map(|new_id_arg| {
                CppType::Object(format_wayland_interface_to_cpp_class(
                    new_id_arg
                        .interface
                        .as_ref()
                        .expect("NewId is missing interface"),
                ))
            });
        let mut virtual_method = CppMethod::new(
            method.name.as_str(),
            virtual_retval,
            true,
            true,
            true,
            false,
        );
        for wayland_arg in method
            .args
            .iter()
            .filter(|arg| arg.type_ != WaylandArgType::NewId)
        {
            let cpp_arg = wayland_arg_to_cpp_arg(wayland_arg);
            match cpp_arg.cpp_type() {
                CppType::Object(type_name) => {
                    virtual_method.add_arg(CppArg::new(
                        CppType::Weak(type_name.clone()),
                        wayland_arg.name.as_str(),
                        wayland_arg.allow_null.unwrap_or(false),
                    ));
                }
                _ => {
                    virtual_method.add_arg(cpp_arg);
                }
            }
        }

        vec![cpp_method, virtual_method]
    } else {
        vec![cpp_method]
    }
}

fn wayland_event_to_cpp_method(event: &WaylandEvent) -> CppMethod {
    let mut cpp_method = CppMethod::new(
        format!("send_{}_event", event.name),
        None,
        false,
        false,
        false,
        true,
    );
    let args = event.args.iter().map(wayland_arg_to_cpp_arg);

    let sanitized_args: Vec<String> = args
        .clone()
        .flat_map(|arg| {
            let arg_name = match arg.cpp_type() {
                CppType::Object(_) => format!("{}->get_box()", sanitize_identifier(&arg.name())),
                _ => sanitize_identifier(&arg.name()),
            };
            if let Some(has_name) = arg.has_name() {
                vec![arg_name, has_name]
            } else {
                vec![arg_name]
            }
        })
        .collect();

    for arg in args {
        cpp_method.add_arg(arg);
    }

    cpp_method.set_body(format!(
        "if (!instance_.has_value()) {{\n    throw \"Wayland event sender used before associate()\";\n}}\ninstance_.value()->{}({});",
        event.name,
        sanitized_args.join(", ")
    ));
    cpp_method
}

fn write_wayland_server_generated(protocols: &Vec<WaylandProtocol>) {
    let tokens = generate_wayland_server_generated_rs(&protocols);
    write_generated_rust_file(tokens, "wayland_server_generated.rs");
}
