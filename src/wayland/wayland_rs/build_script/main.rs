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
mod ffi_generation;
mod helpers;
mod protocol_middleware_generation;
mod protocol_parser;

use cpp_builder::{
    sanitize_identifier, CppArg, CppBuilder, CppClass, CppEnum, CppEnumOption, CppMethod,
    CppNamespace, CppType,
};
use ffi_generation::generate_ffi;
use helpers::*;
use proc_macro2::TokenStream;
use protocol_middleware_generation::generate_wayland_interface_middleware;
use protocol_parser::{
    parse_protocols, InterfaceItem, WaylandArg, WaylandArgType, WaylandEnum, WaylandEvent,
    WaylandInterface, WaylandProtocol, WaylandRequest,
};
use quote::{format_ident, quote};
use std::{env, path::Path};
use syn::Ident;

fn main() {
    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let include_path = Path::new(&manifest_dir).join(".");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/wayland_server.rs");

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

    cxx_build::bridges(vec!["src/ffi.rs"])
        .include(&include_path)
        .compile("wayland_rs");
}

fn write_protocols_rs(protocols: &Vec<WaylandProtocol>) {
    let generated_protocols = protocols.iter().map(|protocol| {
        // We rely on the wayland_server crate for the core Wayland protocol.
        if protocol.name == "wayland" {
            return quote! {};
        }

        let struct_name = dash_to_snake_ident(&protocol.name);
        let path = &protocol.path;

        // Add use statements for other protocol dependencies.
        let protocol_dependencies: std::collections::HashSet<String> = protocol
            .dependencies
            .iter()
            .filter_map(|dependency| {
                protocols
                    .iter()
                    .find(|dep_protocol| {
                        dep_protocol.name != protocol.name
                            && dep_protocol
                                .interfaces
                                .iter()
                                .any(|iface| iface.name == *dependency)
                    })
                    .map(|dep_protocol| dep_protocol.name.clone())
            })
            .collect();

        let mut server_code_use_statements = quote! {
            use super::wayland_server;
            use self::interfaces::*;
        };

        for dependency in &protocol_dependencies {
            let dep_struct_name = dash_to_snake_ident(dependency);
            if dep_struct_name != struct_name {
                if dep_struct_name == "wayland" {
                    server_code_use_statements = quote! {
                        #server_code_use_statements
                        use wayland_server::protocol::*;
                    };
                } else {
                    server_code_use_statements = quote! {
                        #server_code_use_statements
                        use super::#dep_struct_name::*;
                    };
                }
            }
        }

        let mut interface_code_use_statements: TokenStream = quote! {};
        for dependency in &protocol_dependencies {
            let dep_struct_name = dash_to_snake_ident(dependency);
            if dep_struct_name != struct_name {
                if dep_struct_name == "wayland" {
                    interface_code_use_statements = quote! {
                        #interface_code_use_statements
                        use wayland_server::protocol::__interfaces::*;
                    };
                } else {
                    // Note: #[allow(unused_imports)] is used here to suppress warnings in case some protocols
                    // do not actually use any interfaces from their dependencies. This only happens in a
                    // input-method-unstable-v2 because it uses the enums from text-input-unstable-v3 but not
                    // the interface name or anything else.
                    //
                    // This is much simpler than piping the data through the protocol parser, so let's keep things
                    // straightforward for now.
                    interface_code_use_statements = quote! {
                        #interface_code_use_statements

                        #[allow(unused_imports)]
                        use super::super::#dep_struct_name::interfaces::*;
                    };
                }
            }
        }

        quote! {
            pub mod #struct_name {

                #server_code_use_statements

                pub mod interfaces {
                    #interface_code_use_statements
                    wayland_scanner::generate_interfaces!(#path);
                }

                wayland_scanner::generate_server_code!(#path);
            }
        }
    });

    let generated_protocol_rs = quote! {
        #[allow(dead_code, unused_imports)]
        mod protocols {
            use wayland_server;

            #(#generated_protocols)*
        }
    };

    write_generated_rust_file(generated_protocol_rs, "protocols.rs");
}

/// Generate the namespace token for a protocol (either wayland_server::protocol or protocols::module_name).
fn generate_namespace(protocol: &WaylandProtocol) -> TokenStream {
    if protocol.name == "wayland" {
        quote! { wayland_server::protocol }
    } else {
        let protocol_module = dash_to_snake_ident(&protocol.name);
        quote! { protocols::#protocol_module }
    }
}

/// Generate a GlobalDispatch implementation for a single interface.
fn generate_global_dispatch_impl(
    interface: &protocol_parser::WaylandInterface,
    namespace_name: &TokenStream,
) -> TokenStream {
    let interface_name = dash_to_snake_ident(&interface.name.to_string());

    if interface_name == "wl_display" {
        // wl_display is handled specially in wayland_server crate via the 'Display' struct.
        return quote! {};
    }

    let interface_struct_name = format_ident!("{}", snake_to_pascal(&interface.name));
    let ext_interface_struct_name = format_ident!("{}Ext", snake_to_pascal(&interface.name));
    let create_global_method = format_ident!("create_{}", &interface.name);
    quote! {
        impl GlobalDispatch<#namespace_name::#interface_name::#interface_struct_name, Arc<Mutex<ffi::GlobalFactory>>>
            for ServerState
        {
            fn bind(
                _state: &mut Self,
                _handle: &wayland_server::DisplayHandle,
                _client: &wayland_server::Client,
                resource: New<#namespace_name::#interface_name::#interface_struct_name>,
                global_data: &Arc<Mutex<ffi::GlobalFactory>>,
                data_init: &mut wayland_server::DataInit<'_, Self>,
            ) {
                use crate::ffi;
                let mut guard = global_data.lock().unwrap();
                let global = unsafe {
                    ::std::pin::Pin::new_unchecked(&mut *guard)
                }.#create_global_method();
                let arc = Arc::new(Mutex::new(global));
                let instance = data_init.init(resource, arc.clone());
                let boxed = Box::new(crate::middleware::#ext_interface_struct_name{ wrapped: instance });
                let mut guard = arc.lock().unwrap();
                guard.pin_mut().associate(boxed);
            }
        }
    }
}

/// Generate a TokenStream that transforms a single argument for crossing the Rust/C++ boundary.
fn transform_argument_for_cpp(arg: &WaylandArg) -> Option<TokenStream> {
    let arg_name = format_ident!("{}", arg.name);

    // Enums are provided as WlEnum, so we need to cast them to u32 so that
    // they can be sent across the barrier.
    if arg.enum_.is_some() {
        return match arg.type_ {
            WaylandArgType::Uint => Some(quote! {
                let #arg_name = u32::from(#arg_name);
            }),
            _ => None,
        };
    }

    match arg.type_ {
        // The Wayland Rust crate will provide us with raw Wayland Rust objects.
        // C++ expects the shared_ptr data that backs these rust objects to be sent
        // as parameters for objects.
        WaylandArgType::Object => {
            let arg_type = format_ident!(
                "{}Impl",
                snake_to_pascal(arg.interface.clone().unwrap().as_str())
            );
            if arg.allow_null.unwrap_or(false) {
                let null_ptr_name = format_ident!("__{}_null_ptr", arg.name.replace('-', "_"));
                Some(quote! {
                    let #null_ptr_name = cxx::UniquePtr::<ffi::#arg_type>::null();
                    let #arg_name: &cxx::UniquePtr<ffi::#arg_type> = match #arg_name.as_ref() {
                        Some(o) => o.data().unwrap(),
                        None => &#null_ptr_name
                    };
                })
            } else {
                Some(quote! {
                    let #arg_name: &cxx::UniquePtr<ffi::#arg_type> = #arg_name.data().unwrap();
                })
            }
        }
        WaylandArgType::Fd => Some(quote! {
            let #arg_name = #arg_name.as_raw_fd();
        }),
        WaylandArgType::String if arg.allow_null.unwrap_or(false) => Some(quote! {
            let #arg_name = match #arg_name {
                Some(o) => o,
                None => String::new()
            };
        }),
        _ => None,
    }
}

/// Generate the body of a request handler arm.
///
/// Requests come in two distinct forms:
/// 1. Regular requests that ferry their data directly into the C++ method.
/// 2. Resource creation requests which ask the C++ interface to create
///    the resource for them. These methods MUST call data_init.init()
///    with the newly created C++ resource.
fn generate_request_body(request: &WaylandRequest) -> TokenStream {
    let snake_request_name = dash_to_snake_ident(&sanitize_identifier(request.name.as_str()));
    let new_id_arg = request
        .args
        .iter()
        .find(|arg| arg.type_ == WaylandArgType::NewId);

    if let Some(new_id_arg) = new_id_arg {
        let new_id_name = format_ident!("{}", new_id_arg.name);
        let call_arg_names: Vec<TokenStream> = request
            .args
            .iter()
            .filter(|arg| arg.type_ != WaylandArgType::NewId)
            .map(|arg| {
                let arg_name = format_ident!("{}", arg.name.as_str());
                quote! { #arg_name }
            })
            .collect();

        quote! {
            let mut guard = data.lock().unwrap();
            let child = unsafe {
                ::std::pin::Pin::new_unchecked(&mut *guard)
            }.pin_mut().#snake_request_name(#( #call_arg_names ),*);
            data_init.init(#new_id_name, Arc::new(Mutex::new(child)));
        }
    } else {
        let call_arg_names: Vec<TokenStream> = request
            .args
            .iter()
            .map(|arg| {
                let arg_name = format_ident!("{}", arg.name.as_str());
                quote! { #arg_name }
            })
            .collect();

        quote! {
            let mut guard = data.lock().unwrap();
            unsafe {
                ::std::pin::Pin::new_unchecked(&mut *guard)
            }.pin_mut().#snake_request_name(#( #call_arg_names ),*);
        }
    }
}

/// Generate request handler arms for an interface's requests.
fn generate_request_handler_arms(
    interface: &protocol_parser::WaylandInterface,
    namespace_name: &TokenStream,
    interface_name: &Ident,
) -> Vec<TokenStream> {
    interface
        .items
        .iter()
        .filter_map(|item| {
            let protocol_parser::InterfaceItem::Request(request) = item else {
                return None;
            };

            let request_name = format_ident!("{}", snake_to_pascal(&request.name));
            let arg_names: Vec<TokenStream> = request
                .args
                .iter()
                .map(|arg| {
                    let arg_name = format_ident!("{}", arg.name.as_str());
                    quote! { #arg_name }
                })
                .collect();

            let transformed_args: Vec<TokenStream> = request
                .args
                .iter()
                .filter_map(transform_argument_for_cpp)
                .collect();

            let body = generate_request_body(request);

            Some(quote! {
                #namespace_name::#interface_name::Request::#request_name { #( #arg_names ),* } => {
                    #( #transformed_args )*
                    #body
                }
            })
        })
        .collect()
}

/// Generate a Dispatch implementation for a single interface.
fn generate_dispatch_impl(
    interface: &protocol_parser::WaylandInterface,
    namespace_name: &TokenStream,
    is_wayland_protocol: bool,
) -> TokenStream {
    let interface_name = dash_to_snake_ident(&interface.name);

    if interface_name == "wl_display" || interface_name == "wl_registry" {
        // wl_display and wl_registry are handled specially in wayland_server crate via the 'Display' struct.
        return quote! {};
    }

    let protocol_struct_name = format_ident!("{}", snake_to_pascal(&interface.name));
    let ext_struct_name = format_ident!("{}Impl", snake_to_pascal(&interface.name));

    let mut request_handler_arms =
        generate_request_handler_arms(interface, namespace_name, &interface_name);

    // If the interface comes from wayland, we need to generate an empty arm because the
    // enum is marked as non-exhaustive.
    if is_wayland_protocol {
        request_handler_arms.push(quote! {
            _ => {}
        });
    }

    // This snippet checks if any request on the interface creates a new Wayland object.
    // If none do, then we can add the underscore in front of _data_init to show that it is
    // not used.
    let interface_creates_wayland_objects = interface.items.iter().any(|item| match item {
        InterfaceItem::Request(request) => request
            .args
            .iter()
            .any(|arg| arg.type_ == WaylandArgType::NewId),
        _ => false,
    });

    let data_init_name = format_ident!(
        "{}",
        if interface_creates_wayland_objects {
            "data_init"
        } else {
            "_data_init"
        }
    );

    // This snippet checks if the interface has any requests at all. If not, then data
    // will be prefixed with an underscore.
    let interface_has_requests = interface.items.iter().any(|item| match item {
        InterfaceItem::Request(_) => true,
        _ => false,
    });

    let data_name = format_ident!(
        "{}",
        if interface_has_requests {
            "data"
        } else {
            "_data"
        }
    );

    quote! {
        unsafe impl Send for ffi::#ext_struct_name {}
        unsafe impl Sync for ffi::#ext_struct_name {}

        impl Dispatch<#namespace_name::#interface_name::#protocol_struct_name, Arc<Mutex<cxx::UniquePtr<ffi::#ext_struct_name>>>>
            for ServerState
        {
            fn request(
                _state: &mut Self,
                _client: &Client,
                _resource: &#namespace_name::#interface_name::#protocol_struct_name,
                request: <#namespace_name::#interface_name::#protocol_struct_name as wayland_server::Resource>::Request,
                #data_name: &Arc<Mutex<cxx::UniquePtr<ffi::#ext_struct_name>>>,
                _dhandle: &DisplayHandle,
                #data_init_name: &mut DataInit<'_, Self>,
            ) {
                match request {
                    #(#request_handler_arms),*
                }
            }
        }
    }
}

/// Generate all dispatch implementations (both GlobalDispatch and Dispatch) for a single protocol.
fn generate_dispatch_implementations(protocol: &WaylandProtocol) -> TokenStream {
    let namespace_name = generate_namespace(protocol);

    let global_interfaces: Vec<&protocol_parser::WaylandInterface> = protocol
        .interfaces
        .iter()
        .filter(|interface| interface.is_global)
        .collect();

    let global_dispatch_impls = global_interfaces
        .iter()
        .map(|interface| generate_global_dispatch_impl(interface, &namespace_name));

    let is_wayland_protocol = protocol.name == "wayland";
    let dispatch_impls = protocol
        .interfaces
        .iter()
        .map(|interface| generate_dispatch_impl(interface, &namespace_name, is_wayland_protocol));

    quote! {
        #(#global_dispatch_impls)*
        #(#dispatch_impls)*
    }
}

fn write_dispatch_rs(protocols: &Vec<WaylandProtocol>) {
    let generated_dispatch_implementations = protocols
        .iter()
        .map(|protocol| generate_dispatch_implementations(protocol));

    let generated_protocol_rs = quote! {
        #[allow(dead_code, unused_imports)]
        mod dispatch {
            use wayland_server::{Client, DataInit, Dispatch, GlobalDispatch, New, DisplayHandle, Resource};
            use crate::protocols;
            use crate::wayland_server::ServerState;
            use crate::ffi;
            use std::os::fd::{AsRawFd, RawFd};
            use std::sync::{Arc, Mutex};

            #(#generated_dispatch_implementations)*
        }
    };

    write_generated_rust_file(generated_protocol_rs, "dispatch.rs");
}

fn create_global_factory(protocols: &Vec<WaylandProtocol>) -> CppBuilder {
    let mut builder: CppBuilder = CppBuilder::new(
        "MIR_WAYLANDRS_GLOBALS".to_string(),
        "global_factory".to_string(),
    );
    builder.add_include("<memory>".to_string());
    let mut namespace = CppNamespace::new(vec!["mir".to_string(), "wayland_rs".to_string()]);
    let mut class = CppClass::new("GlobalFactory".to_string());
    protocols.iter().for_each(|protocol| {
        protocol
            .interfaces
            .iter()
            .filter(|interface| interface.is_global)
            .filter(|interface| interface.name != "wl_display" && interface.name != "wl_registry")
            .for_each(|global_interface| {
                let class_name = format_cpp_class_name(&global_interface.name);
                namespace.add_forward_declaration_class(&class_name);

                let method = CppMethod::new(
                    format!("create_{}", global_interface.name),
                    Some(CppType::Object(class_name)),
                    true,
                );
                class.add_method(method);
            })
    });
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

    // Generate ffi_fwd.h first so that protocol headers can include it without
    // creating a circular dependency with the CXX-generated ffi.rs.h.
    let ffi_fwd_builder = create_ffi_fwd_builder(protocols);
    write_cpp_header(&ffi_fwd_builder);

    let mut builders: Vec<CppBuilder> = protocols
        .iter()
        .map(|protocol| create_cpp_builder(protocol))
        .collect();
    builders.push(global_builder);

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
    let mut builder = CppBuilder::new("MIR_WAYLANDRS_FFI_FWD".to_string(), "ffi_fwd".to_string());
    // <rust/cxx.h> is included here so that protocol headers pulling in ffi_fwd.h
    // have access to rust::Box, rust::String, etc. without a direct dependency on ffi.rs.h.
    builder.add_include("<rust/cxx.h>".to_string());
    let mut namespace = CppNamespace::new(vec!["mir".to_string(), "wayland_rs".to_string()]);

    // WaylandServer is declared in ffi.rs but used nowhere in the protocol headers;
    // include it for completeness so all Rust types are forward-declared.
    namespace.add_forward_declaration_class("WaylandServer");

    for protocol in protocols {
        for interface in &protocol.interfaces {
            if interface.name == "wl_registry" || interface.name == "wl_display" {
                continue;
            }
            namespace.add_forward_declaration_class(&format_rust_ext_struct_name(&interface.name));
        }
    }

    builder.add_namespace(namespace);
    builder
}

fn create_cpp_builder(protocol: &WaylandProtocol) -> CppBuilder {
    let guard = format!("MIR_WAYLANDRS_{}", protocol.name.to_uppercase());
    let mut builder = CppBuilder::new(guard, protocol.name.clone());
    let mut namespace = CppNamespace::new(vec!["mir".to_string(), "wayland_rs".to_string()]);

    let classes = protocol
        .interfaces
        .iter()
        .filter(|interface| interface.name != "wl_registry" && interface.name != "wl_display")
        .map(|interface| wayland_interface_to_cpp_class(interface));

    for class in classes {
        namespace.add_class(class);
    }
    for interface in &protocol.dependencies {
        let class_name = format_cpp_class_name(interface);
        namespace.add_forward_declaration_class(&class_name);
    }
    builder.add_namespace(namespace);
    // Use ffi_fwd.h (generated alongside the protocol headers) instead of the
    // CXX-generated ffi.rs.h to avoid a circular include dependency:
    //   ffi.rs.h  →  include/*.h  →  ffi.rs.h
    builder.add_include("\"ffi_fwd.h\"".to_string());
    builder.add_include("<memory>".to_string());

    builder
}

fn write_cpp_header(builder: &CppBuilder) {
    let filename = format!("{}.h", builder.filename);
    write_generated_cpp_file(&builder.to_cpp_header(), filename.as_str());
}

fn write_cpp_source(builder: &CppBuilder) {
    let header_filename = format!("{}.h", builder.filename);

    let filename = format!("{}.cpp", builder.filename);
    write_generated_cpp_file(&builder.to_cpp_source(header_filename), filename.as_str());
}

fn wayland_interface_to_cpp_class(interface: &WaylandInterface) -> CppClass {
    let class_name = format_cpp_class_name(&interface.name);
    let mut class = CppClass::new(class_name);
    let methods = interface.items.iter().filter_map(|item| {
        if let protocol_parser::InterfaceItem::Request(request) = item {
            Some(wayland_request_to_cpp_method(request))
        } else if let protocol_parser::InterfaceItem::Event(event) = item {
            Some(wayland_event_to_cpp_method(event))
        } else {
            None
        }
    });

    for enum_ in wayland_interface_to_enums(interface) {
        class.add_enum(enum_);
    }

    // Add the method that associates the boxed rust interface with the C++ class.
    let mut associate_method = CppMethod::new("associate".to_string(), None, true);
    associate_method.add_arg(CppArg {
        cpp_type: CppType::Box(snake_to_pascal(&format!("{}Ext", &interface.name))),
        name: "instance".to_string(),
    });
    class.add_method(associate_method);

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
        result.add_option(CppEnumOption {
            name: snake_to_pascal(option.name.as_str()),
            value: option.value as u32,
        });
    }

    result
}

fn wayland_arg_to_cpp_arg(arg: &WaylandArg) -> CppArg {
    let type_ = match arg.type_ {
        WaylandArgType::Int => CppType::CppI32,
        WaylandArgType::Uint => CppType::CppU32,
        WaylandArgType::Fixed => CppType::CppF64,
        WaylandArgType::String => CppType::String,
        WaylandArgType::Object => CppType::Object(format_cpp_class_name(
            &arg.interface.clone().expect("Object is missing interface"),
        )),
        WaylandArgType::Array => CppType::Array,
        WaylandArgType::Fd => CppType::Fd,
        _ => panic!("Unhandled argument type"),
    };

    CppArg::new(type_, arg.name.clone())
}

fn wayland_request_to_cpp_method(method: &WaylandRequest) -> CppMethod {
    // Methods will have a return value in C++ if they are creating a new Wayland object.
    // Otherwise they always return void.
    let retval = match method
        .args
        .iter()
        .find(|arg| arg.type_ == WaylandArgType::NewId)
    {
        Some(new_id_arg) => {
            let name = format_cpp_class_name(&new_id_arg.interface.clone().unwrap());
            Some(CppType::Object(name))
        }
        None => None,
    };

    let mut cpp_method = CppMethod::new(method.name.clone(), retval, true);
    let args = method
        .args
        .iter()
        .filter(|arg| arg.type_ != WaylandArgType::NewId)
        .map(wayland_arg_to_cpp_arg);

    for arg in args {
        cpp_method.add_arg(arg);
    }

    cpp_method
}

fn wayland_event_to_cpp_method(event: &WaylandEvent) -> CppMethod {
    let mut cpp_method = CppMethod::new(format!("send_{}", event.name), None, false);
    let args = event
        .args
        .iter()
        .filter(|arg| arg.type_ != WaylandArgType::NewId)
        .map(wayland_arg_to_cpp_arg);

    for arg in args {
        cpp_method.add_arg(arg);
    }

    cpp_method
}
