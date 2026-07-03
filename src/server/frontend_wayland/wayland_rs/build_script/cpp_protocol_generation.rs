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

use crate::cpp_builder::{
    sanitize_identifier, CppArg, CppBuilder, CppClass, CppEnum, CppEnumOption, CppMethod,
    CppNamespace, CppType,
};
use crate::helpers::{
    format_wayland_interface_to_cpp_class, format_wayland_interface_to_rust_extension_struct,
    snake_to_pascal,
};
use crate::protocol_parser::{
    WaylandArg, WaylandArgType, WaylandEnum, WaylandEvent, WaylandInterface, WaylandProtocol,
    WaylandRequest,
};

/// The output of CPP protocol generation: the per-protocol builders,
/// the global factory builder, and the FFI forward-declaration builder.
pub struct CppProtocolGenerationOutput {
    /// The FFI forward-declaration builder (ffi_fwd.h).
    pub ffi_fwd_builder: CppBuilder,
    /// Per-protocol builders plus the global factory and notification handler.
    pub builders: Vec<CppBuilder>,
}

/// Generate the C++ protocol builders for all parsed Wayland protocols.
///
/// This produces:
/// - A `CppBuilder` per protocol (containing abstract classes per-interface)
/// - A global factory builder
/// - A wayland server notification handler builder
/// - A work callback builder
/// - An fd-ready callback builder
/// - An FFI forward-declaration builder
pub fn generate_cpp_protocol_builders(
    protocols: &Vec<WaylandProtocol>,
) -> CppProtocolGenerationOutput {
    let global_builder = create_global_factory(protocols);
    let wayland_server_notification_handler_builder = create_wayland_server_notification_handler();
    let work_callback_builder = create_work_callback();
    let fd_ready_callback_builder = create_fd_ready_callback();
    let ffi_fwd_builder = create_ffi_fwd_builder(protocols);

    let mut builders: Vec<CppBuilder> = protocols.iter().map(create_cpp_builder).collect();
    builders.push(global_builder);
    builders.push(wayland_server_notification_handler_builder);
    builders.push(work_callback_builder);
    builders.push(fd_ready_callback_builder);

    CppProtocolGenerationOutput {
        ffi_fwd_builder,
        builders,
    }
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
                let ext_name =
                    format_wayland_interface_to_rust_extension_struct(&global_interface.name);
                namespace.add_forward_declaration_class(&class_name);
                namespace.add_forward_declaration_struct(&snake_to_pascal(&ext_name));

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
                method.add_arg(CppArg::new(
                    CppType::Box(snake_to_pascal(&ext_name)),
                    "instance",
                    false,
                ));
                method.add_arg(CppArg::new(CppType::CppU32, "object_id", false));
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

/// Create the `WorkCallback` interface.
///
/// This is the Rust -> C++ callback used to drain the units of work that were
/// scheduled onto the Wayland event loop. The work itself lives on the C++
/// side (a queue of `std::function`s); Rust calls `execute()` on the
/// event-loop thread to run whatever C++ work is currently pending.
fn create_work_callback() -> CppBuilder {
    let mut builder: CppBuilder = CppBuilder::new("MIR_WAYLANDRS_WORK_CALLBACK", "work_callback");

    builder.add_cpp_include("\"wayland_rs/src/ffi.rs.h\"");
    let mut namespace = CppNamespace::new(vec!["mir", "wayland_rs"]);
    let mut class = CppClass::new("WorkCallback");

    let execute_method = CppMethod::new("execute", None, true, false, true, true);
    class.add_method(execute_method);

    namespace.add_class(class);
    builder.add_namespace(namespace);
    builder
}

/// Create the `FdReadyCallback` interface.
///
/// This is the Rust -> C++ callback used to notify C++ that a file descriptor
/// it registered (via `WaylandServer::register_fd_ready_listener`) has become
/// readable. The registration is one-shot: Rust calls `ready()` on the
/// event-loop thread the first time the descriptor is ready for reading, then
/// stops watching it.
fn create_fd_ready_callback() -> CppBuilder {
    let mut builder: CppBuilder =
        CppBuilder::new("MIR_WAYLANDRS_FD_READY_CALLBACK", "fd_ready_callback");

    builder.add_cpp_include("\"wayland_rs/src/ffi.rs.h\"");
    let mut namespace = CppNamespace::new(vec!["mir", "wayland_rs"]);
    let mut class = CppClass::new("FdReadyCallback");

    let ready_method = CppMethod::new("ready", None, true, false, true, true);
    class.add_method(ready_method);

    namespace.add_class(class);
    builder.add_namespace(namespace);
    builder
}
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
    // WaylandClient is used in the protected constructor of every generated C++ implementation.
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
        .map(wayland_interface_to_cpp_class);

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
    builder.add_header_include("<cstdint>");
    builder.add_header_include("<string>");
    builder.add_header_include("<optional>");
    builder.add_header_include("<rust/cxx.h>");

    builder.add_cpp_include("\"wayland_rs/src/ffi.rs.h\"");

    builder
}

fn wayland_interface_to_cpp_class(interface: &WaylandInterface) -> CppClass {
    let class_name = format_wayland_interface_to_cpp_class(&interface.name);
    let mut class = CppClass::new(class_name.clone());
    class.set_superclass("LifetimeTracker");

    // Backwards-compatible recovery helper, replacing the legacy `Concrete::from(wl_resource*)`.
    // The generator only knows this abstract base, not the hand-written concrete subclass, so the
    // method is templated on the caller's concrete type `Self`. It downcasts the weak's referent
    // (preserving the destroyed-flag) and returns a `Self*`, or nullptr if the weak is
    // null/expired or does not refer to a `Self`. Usage: `Surface::from<WlSurfaceImpl>(weak)`.
    class.add_raw_public_decl(format!(
        "    template<typename Self>\n\
         \x20   static auto from(::mir::wayland_rs::Weak<{class_name}> const& weak) -> Self*\n\
         \x20   {{\n\
         \x20       return ::mir::wayland_rs::as_nullable_ptr(\n\
         \x20           ::mir::wayland_rs::weak_cast<Self>(weak));\n\
         \x20   }}",
        class_name = class_name
    ));

    let methods = interface
        .items
        .iter()
        .filter_map(|item| {
            if let crate::protocol_parser::InterfaceItem::Request(request) = item {
                Some(wayland_request_to_cpp_method(request))
            } else if let crate::protocol_parser::InterfaceItem::Event(event) = item {
                Some(wayland_event_to_cpp_methods(event))
            } else {
                None
            }
        })
        .flatten();

    for enum_ in wayland_interface_to_enums(interface) {
        class.add_enum(enum_);
    }

    // Declare the corresponding private members.
    class.add_private_member(CppArg::new(
        CppType::Box(snake_to_pascal(
            &format_wayland_interface_to_rust_extension_struct(&interface.name),
        )),
        "instance_",
        false,
    ));
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

    // Add the "get_box" method that will return the boxed rust interface. This is used
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
    get_box_method.set_body("return instance_;");
    class.add_method(get_box_method);

    // Add a post_error method that posts a protocol error to the client for this resource.
    // This follows the same pattern as event-sending methods, routing through the Rust
    // middleware Ext type to call wayland-server's DisplayHandle::post_error().
    let mut post_error_method = CppMethod::new("post_error", None, false, false, false, true);
    post_error_method.add_arg(CppArg::new(CppType::CppU32, "code", false));
    post_error_method.add_arg(CppArg::new(CppType::String, "message", false));
    post_error_method.set_body("instance_->post_error(code, message);");
    class.add_method(post_error_method);

    // Add a destroy_and_delete method for server-initiated resource destruction.
    // This is the equivalent of wl_resource_destroy() in the old C API: it destroys the
    // underlying Wayland resource, which triggers the Dispatch::destroyed() callback,
    // unregisters the resource, and allows the C++ object to be dropped once all shared owners release it.
    let mut destroy_and_delete_method =
        CppMethod::new("destroy_and_delete", None, false, false, false, true);
    destroy_and_delete_method.set_body("instance_->destroy_and_delete();");
    class.add_method(destroy_and_delete_method);

    // Add a protected constructor and member so that subclasses know which client
    // they are serving and can interact with it as they please.
    class.add_protected_constructor_arg(CppArg::new(
        CppType::Box("WaylandClient".to_string()),
        "client",
        false,
    ));
    // Add instance_ and object_id_ constructor args after client to match declaration order
    // (public members are declared before private members in the generated header).
    class.add_protected_constructor_arg(CppArg::new(
        CppType::Box(snake_to_pascal(
            &format_wayland_interface_to_rust_extension_struct(&interface.name),
        )),
        "instance_",
        false,
    ));
    class.add_protected_constructor_arg(CppArg::new(CppType::CppU32, "object_id_", false));
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
        if let crate::protocol_parser::InterfaceItem::Enum(enum_) = item {
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

    let cpp_args: Vec<CppArg> = method
        .args
        .iter()
        .filter(|arg| arg.type_ != WaylandArgType::NewId)
        .map(wayland_arg_to_cpp_arg)
        .collect();

    let has_retval = retval.is_some();

    // If the method creates a child object (has NewId), we need to pass the child's
    // middleware Box and object_id so the child is fully initialized at construction.
    let new_id_ext_args: Vec<CppArg> = if let Some(new_id_arg) = method
        .args
        .iter()
        .find(|arg| arg.type_ == WaylandArgType::NewId)
    {
        let ext_name = format_wayland_interface_to_rust_extension_struct(
            new_id_arg
                .interface
                .as_ref()
                .expect("NewId is missing interface"),
        );
        vec![
            CppArg::new(
                CppType::Box(snake_to_pascal(&ext_name)),
                "child_instance",
                false,
            ),
            CppArg::new(CppType::CppU32, "child_object_id", false),
        ]
    } else {
        vec![]
    };

    let is_destructor = method
        .type_
        .as_deref()
        .map(|type_| type_ == "destructor")
        .unwrap_or(false);

    // A C++ argument sent from Rust as a shared_ptr (an Object) must not be handed to the
    // protocol implementer directly: only the Rust Wayland frontend should own its lifetime.
    // Instead we wrap it in a "Weak" construct so the implementer can still interact with
    // other protocol implementations without muddying lifetimes.
    //
    // Additionally, nullable (`allow_null`) arguments arrive as a `(value, has_value)` pair,
    // since `std::optional` cannot cross the FFI boundary. We do not want that boolean to
    // leak into the public API, so we present such arguments as `std::optional<...>`.
    //
    // In either case the public surface differs from the FFI surface, so we:
    // 1. Generate a virtual method (overridden by implementers) using Weak/optional args.
    // 2. Make the FFI-facing handler non-virtual and delegate to the virtual method,
    //    performing the Weak-wrapping and optional-packaging in between.
    let needs_split = !is_destructor
        && cpp_args
            .iter()
            .any(|arg| matches!(arg.cpp_type(), CppType::Object(_)) || arg.has_name().is_some());

    let mut cpp_method =
        CppMethod::new(method.name.as_str(), retval, !needs_split, true, true, true);
    for arg in &cpp_args {
        cpp_method.add_arg(arg.clone());
    }
    for arg in new_id_ext_args.clone() {
        cpp_method.add_arg(arg);
    }

    if is_destructor {
        cpp_method.set_body("");
    }

    if !needs_split {
        return vec![cpp_method];
    }

    // Build the FFI-facing body: package each argument into its public form, then delegate
    // to the virtual overload.
    let return_prefix = if has_retval { "return " } else { "" };
    let mut all_delegation_args: Vec<String> = cpp_args
        .iter()
        .map(delegation_argument_expression)
        .collect();

    // Forward child_instance and child_object_id to the virtual method
    for arg in &new_id_ext_args {
        all_delegation_args.push(format!("std::move({})", arg.name()));
    }
    let delegation_call = format!(
        "{}{}({});",
        return_prefix,
        sanitize_identifier(&method.name),
        all_delegation_args.join(", ")
    );
    cpp_method.set_body(delegation_call);

    // Generate the virtual method that protocol implementers override.
    // This method uses Weak args instead of shared_ptr args, and std::optional for
    // nullable args.
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
    for arg in &cpp_args {
        virtual_method.add_arg(public_argument(arg));
    }
    // Add the child middleware args to the virtual method too
    for arg in new_id_ext_args {
        virtual_method.add_arg(arg);
    }

    vec![cpp_method, virtual_method]
}

/// Returns the public-facing `CppArg` for a request argument: Object args become `Weak`,
/// and nullable args are wrapped in `std::optional`, dropping the `has_X` boolean.
fn public_argument(arg: &CppArg) -> CppArg {
    let public_type = match arg.cpp_type() {
        CppType::Object(type_name) => CppType::Weak(type_name.clone()),
        other => other.clone(),
    };
    if arg.has_name().is_some() {
        CppArg::new(CppType::Optional(Box::new(public_type)), arg.name(), false)
    } else {
        CppArg::new(public_type, arg.name(), false)
    }
}

/// Builds the expression used to forward an argument from the FFI-facing handler to the
/// virtual method that protocol implementers override.
fn delegation_argument_expression(arg: &CppArg) -> String {
    let name = arg.name();
    match (arg.cpp_type(), arg.has_name()) {
        (CppType::Object(type_name), Some(has_name)) => format!(
            "{has_name} ? std::make_optional(wayland_rs::Weak<{type_name}>({name})) : std::nullopt"
        ),
        (CppType::Object(type_name), None) => {
            format!("wayland_rs::Weak<{type_name}>({name})")
        }
        (_, Some(has_name)) => {
            format!("{has_name} ? std::make_optional(std::move({name})) : std::nullopt")
        }
        (_, None) => name.to_string(),
    }
}

fn wayland_event_to_cpp_methods(event: &WaylandEvent) -> Vec<CppMethod> {
    let since = event.since.unwrap_or(1);
    let needs_version_guard = since > 1;

    let cpp_args: Vec<CppArg> = event.args.iter().map(wayland_arg_to_cpp_arg).collect();

    // Build the arguments forwarded across the FFI boundary to the Rust middleware. Nullable
    // values are unpacked here from their public `std::optional` form into the
    // `(value, has_value)` (or raw-pointer, for objects) representation the FFI expects.
    let ffi_call_args: Vec<String> = cpp_args
        .iter()
        .flat_map(event_ffi_call_expressions)
        .collect();

    let send_call = format!("instance_->{}({});", event.name, ffi_call_args.join(", "));

    // Generate the `send_x_event` method
    let send_body = if needs_version_guard {
        format!(
            "if (instance_->version() < {since}) {{\n    post_error(0, \"Tried to send {event_name} event to object with version \" + std::to_string(instance_->version()) + \" (requires version {since})\");\n    return;\n}}\n{send_call}",
            since = since,
            event_name = event.name,
            send_call = send_call,
        )
    } else {
        send_call.clone()
    };

    let mut send_method = CppMethod::new(
        format!("send_{}_event", event.name),
        None,
        false,
        false,
        false,
        false,
    );
    for arg in &cpp_args {
        send_method.add_arg(event_public_argument(arg));
    }
    send_method.set_body(send_body);

    let mut methods = vec![send_method];

    // Generate the `send_x_event_if_supported` method for versioned events
    if needs_version_guard {
        let if_supported_body = format!(
            "if (instance_->version() < {since}) {{\n    return;\n}}\n{send_call}",
            since = since,
            send_call = send_call,
        );

        let mut if_supported_method = CppMethod::new(
            format!("send_{}_event_if_supported", event.name),
            None,
            false,
            false,
            false,
            false,
        );
        for arg in &cpp_args {
            if_supported_method.add_arg(event_public_argument(arg));
        }
        if_supported_method.set_body(if_supported_body);
        methods.push(if_supported_method);
    }

    methods
}

/// Returns the public-facing `CppArg` for an event argument: nullable arguments are wrapped
/// in `std::optional`, dropping the `has_X` boolean.
fn event_public_argument(arg: &CppArg) -> CppArg {
    if arg.has_name().is_some() {
        CppArg::new(
            CppType::Optional(Box::new(arg.cpp_type().clone())),
            arg.name(),
            false,
        )
    } else {
        arg.clone()
    }
}

/// Builds the expression(s) used to forward an event argument from the public
/// `send_x_event` method to the underlying Rust middleware call.
fn event_ffi_call_expressions(arg: &CppArg) -> Vec<String> {
    let name = arg.name();
    match (arg.cpp_type(), arg.has_name()) {
        // A nullable object is forwarded as a raw pointer to its middleware box (or null).
        (CppType::Object(_), Some(_)) => {
            vec![format!(
                "{name}.has_value() ? &(*{name})->get_box() : nullptr"
            )]
        }
        (CppType::Object(_), None) => vec![format!("{name}->get_box()")],
        // A nullable non-object is forwarded as a `(value, has_value)` pair.
        (other, Some(_)) => {
            let empty = match other {
                CppType::String => "std::string{}".to_string(),
                CppType::Array => "std::vector<uint8_t>{}".to_string(),
                _ => format!("std::remove_cvref_t<decltype(*{name})>{{}}", name = name),
            };
            vec![
                format!("{name}.has_value() ? *{name} : {empty}"),
                format!("{name}.has_value()"),
            ]
        }
        (_, None) => vec![name.to_string()],
    }
}
