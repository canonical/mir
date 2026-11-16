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

//! Globals and objects of protocols loaded at runtime.
//!
//! Requests are delivered untyped to C++ `DynamicResourceHandler`s; C++ sends
//! events through a [DynamicResource]. `wayland-backend` panics (aborting Mir)
//! on events that do not match their descriptor, so every event from C++ is
//! fully validated here first.

use super::descriptors::find_interface;
use crate::ffi::{
    DynamicArgument, DynamicArgumentKind, DynamicGlobalHandler, DynamicResourceHandler,
};
use crate::wayland_client::{WaylandClient, WaylandClientId};
use crate::wayland_server_core::ServerState;
use cxx::{SharedPtr, UniquePtr};
use std::error::Error;
use std::ffi::CString;
use std::os::fd::{IntoRawFd, OwnedFd};
use std::sync::{Arc, Mutex, Weak};
use wayland_server::backend::protocol::{
    AllowNull, Argument, ArgumentType, Interface, Message, ANONYMOUS_INTERFACE,
};
use wayland_server::backend::{
    ClientData, ClientId, GlobalHandler, GlobalId, Handle, ObjectData, ObjectId, WeakHandle,
};
use wayland_server::DisplayHandle;

// SAFETY: The C++ handlers are created by C++ and only ever invoked on the
// Wayland event-loop thread; the backend merely requires its object and global
// data to be `Send + Sync` so it can store them.
unsafe impl Send for DynamicGlobalHandler {}
unsafe impl Sync for DynamicGlobalHandler {}
unsafe impl Send for DynamicResourceHandler {}
unsafe impl Sync for DynamicResourceHandler {}

/// Create a global for an interface of a protocol loaded at runtime.
pub(crate) fn create_dynamic_global(
    handle: DisplayHandle,
    interface_name: &str,
    version: u32,
    handler: UniquePtr<DynamicGlobalHandler>,
) -> Result<Box<DynamicGlobal>, Box<dyn Error>> {
    let interface = find_interface(interface_name)
        .ok_or_else(|| format!("Interface \"{interface_name}\" has not been loaded"))?;
    if version == 0 || version > interface.version {
        return Err(format!(
            "Version {version} of \"{interface_name}\" is not supported (maximum {})",
            interface.version
        )
        .into());
    }
    if handler.is_null() {
        return Err("Dynamic global handler is null".into());
    }

    let id = handle.backend_handle().create_global::<ServerState>(
        interface,
        version,
        Arc::new(GlobalData {
            handler: Mutex::new(handler),
        }),
    );

    Ok(Box::new(DynamicGlobal { id, handle }))
}

/// An RAII handle owning a global of a protocol loaded at runtime (see
/// `WaylandServer::create_dynamic_global`).
///
/// Dropping it withdraws the global. Objects already bound remain alive.
pub struct DynamicGlobal {
    id: GlobalId,
    handle: DisplayHandle,
}

impl Drop for DynamicGlobal {
    fn drop(&mut self) {
        self.handle.remove_global::<ServerState>(self.id.clone());
    }
}

struct GlobalData {
    handler: Mutex<UniquePtr<DynamicGlobalHandler>>,
}

impl GlobalHandler<ServerState> for GlobalData {
    fn can_view(
        &self,
        client_id: ClientId,
        _client_data: &Arc<dyn ClientData>,
        _global_id: GlobalId,
    ) -> bool {
        let mut handler = self.handler.lock().unwrap_or_else(|e| e.into_inner());
        let Some(handler) = handler.as_mut() else {
            return false;
        };
        handler
            .can_view(Box::new(WaylandClientId::new(client_id)))
            .unwrap_or_else(|e| {
                log::error!("Dynamic global can_view failed: {}", e.what());
                false
            })
    }

    fn bind(
        self: Arc<Self>,
        handle: &Handle,
        _data: &mut ServerState,
        _client_id: ClientId,
        _global_id: GlobalId,
        object_id: ObjectId,
    ) -> Arc<dyn ObjectData<ServerState>> {
        let object = Arc::new(DynamicObject::default());
        let display_handle = DisplayHandle::from(handle.clone());
        let client = match display_handle.get_client(object_id.clone()) {
            Ok(client) => client,
            Err(_) => return object,
        };
        let resource = Box::new(DynamicResource::new(handle, object_id.clone(), &object));

        let result = {
            let mut handler = self.handler.lock().unwrap_or_else(|e| e.into_inner());
            match handler.as_mut() {
                Some(handler) => handler.bind(
                    Box::new(WaylandClient::new(client, display_handle)),
                    resource,
                ),
                None => return object,
            }
        };

        match result {
            Ok(child_handler) => object.set_handler(child_handler),
            Err(e) => handle_dispatch_error(handle, &object_id, e.what()),
        }
        object
    }
}

/// The backend's per-object data for objects of protocols loaded at runtime.
#[derive(Default)]
struct DynamicObject {
    handler: Mutex<Option<SharedPtr<DynamicResourceHandler>>>,
}

impl DynamicObject {
    fn set_handler(&self, handler: SharedPtr<DynamicResourceHandler>) {
        *self.handler.lock().unwrap_or_else(|e| e.into_inner()) =
            (!handler.is_null()).then_some(handler);
    }

    fn handler(&self) -> Option<SharedPtr<DynamicResourceHandler>> {
        self.handler
            .lock()
            .unwrap_or_else(|e| e.into_inner())
            .clone()
    }
}

impl ObjectData<ServerState> for DynamicObject {
    fn request(
        self: Arc<Self>,
        handle: &Handle,
        _data: &mut ServerState,
        _client_id: ClientId,
        msg: Message<ObjectId, OwnedFd>,
    ) -> Option<Arc<dyn ObjectData<ServerState>>> {
        let mut created: Option<(ObjectId, Arc<DynamicObject>)> = None;
        let args: Vec<DynamicArgument> = msg
            .args
            .into_iter()
            .map(|arg| {
                if let Argument::NewId(id) = &arg {
                    created = Some((id.clone(), Arc::new(DynamicObject::default())));
                }
                incoming_argument(handle, arg)
            })
            .collect();

        let Some(mut handler) = self.handler() else {
            post_implementation_error(
                handle,
                &msg.sender_id,
                &format!(
                    "internal server error: no handler for {}@{}",
                    msg.sender_id.interface().name,
                    msg.sender_id.protocol_id()
                ),
            );
            // The backend requires data for any object the request created.
            return created.map(|(_, child)| child as Arc<dyn ObjectData<ServerState>>);
        };

        // SAFETY: Handlers are only invoked on the event-loop thread, one call at
        // a time (a re-entrant call can only come from the handler itself).
        let handler = unsafe { handler.pin_mut_unchecked() };
        match created {
            None => {
                if let Err(e) = handler.on_request(msg.opcode.into(), args) {
                    handle_dispatch_error(handle, &msg.sender_id, e.what());
                }
                None
            }
            Some((child_id, child)) => {
                let resource = Box::new(DynamicResource::new(handle, child_id, &child));
                match handler.on_constructor_request(msg.opcode.into(), args, resource) {
                    Ok(child_handler) => child.set_handler(child_handler),
                    Err(e) => handle_dispatch_error(handle, &msg.sender_id, e.what()),
                }
                Some(child)
            }
        }
    }

    fn destroyed(
        self: Arc<Self>,
        _handle: &Handle,
        _data: &mut ServerState,
        _client_id: ClientId,
        object_id: ObjectId,
    ) {
        let handler = self
            .handler
            .lock()
            .unwrap_or_else(|e| e.into_inner())
            .take();
        if let Some(mut handler) = handler {
            // SAFETY: as in `request`.
            if let Err(e) = unsafe { handler.pin_mut_unchecked() }.on_destroyed() {
                log::error!(
                    "Error notifying destruction of {}@{}: {}",
                    object_id.interface().name,
                    object_id.protocol_id(),
                    e.what()
                );
            }
        }
    }
}

fn empty_argument(kind: DynamicArgumentKind) -> DynamicArgument {
    DynamicArgument {
        kind,
        int_value: 0,
        uint_value: 0,
        string_value: String::new(),
        is_null: false,
        array_value: Vec::new(),
        fd: -1,
        object_id: 0,
        object_handler: SharedPtr::null(),
    }
}

/// Convert a request argument the backend has already validated against its descriptor.
fn incoming_argument(handle: &Handle, arg: Argument<ObjectId, OwnedFd>) -> DynamicArgument {
    match arg {
        Argument::Int(value) => DynamicArgument {
            int_value: value,
            ..empty_argument(DynamicArgumentKind::Int)
        },
        Argument::Uint(value) => DynamicArgument {
            uint_value: value,
            ..empty_argument(DynamicArgumentKind::Uint)
        },
        Argument::Fixed(value) => DynamicArgument {
            int_value: value,
            ..empty_argument(DynamicArgumentKind::Fixed)
        },
        Argument::Str(value) => match value {
            Some(value) => DynamicArgument {
                string_value: value.to_string_lossy().into_owned(),
                ..empty_argument(DynamicArgumentKind::String)
            },
            None => DynamicArgument {
                is_null: true,
                ..empty_argument(DynamicArgumentKind::String)
            },
        },
        Argument::Object(id) => {
            let object_handler = if id.is_null() {
                None
            } else {
                handle
                    .get_object_data::<ServerState>(id.clone())
                    .ok()
                    .and_then(|data| data.downcast_arc::<DynamicObject>().ok())
                    .and_then(|object| object.handler())
            };
            DynamicArgument {
                object_id: id.protocol_id(),
                object_handler: object_handler.unwrap_or_else(SharedPtr::null),
                ..empty_argument(DynamicArgumentKind::Object)
            }
        }
        Argument::NewId(id) => DynamicArgument {
            object_id: id.protocol_id(),
            ..empty_argument(DynamicArgumentKind::NewId)
        },
        Argument::Array(value) => DynamicArgument {
            array_value: *value,
            ..empty_argument(DynamicArgumentKind::Array)
        },
        Argument::Fd(fd) => DynamicArgument {
            fd: fd.into_raw_fd(),
            ..empty_argument(DynamicArgumentKind::Fd)
        },
    }
}

/// Handle an error from a C++ handler, as the generated dispatch code does.
///
/// `mir::wayland::ProtocolError` encodes itself as
/// "MIR_PROTOCOL_ERROR:<object_id>:<code>: <message>"; anything else is an
/// internal server error.
fn handle_dispatch_error(handle: &Handle, sender: &ObjectId, what: &str) {
    if let Some(encoded) = what.strip_prefix("MIR_PROTOCOL_ERROR:") {
        let (object_id, code, message) = crate::dispatch::parse_post_error(encoded);
        let target = find_object(handle, sender, &ANONYMOUS_INTERFACE, object_id)
            .unwrap_or_else(|| sender.clone());
        handle.post_error(
            target,
            code,
            CString::new(message).unwrap_or_else(|_| c"protocol error".into()),
        );
    } else {
        log::error!(
            "Internal error dispatching Wayland request on {}@{}: {}",
            sender.interface().name,
            sender.protocol_id(),
            what
        );
        post_implementation_error(handle, sender, &format!("internal server error: {what}"));
    }
}

/// Post `wl_display.error` with the `implementation` code to the client owning
/// `object` (see `post_implementation_error` in the generated dispatch code).
fn post_implementation_error(handle: &Handle, object: &ObjectId, message: &str) {
    const WL_DISPLAY_ERROR_IMPLEMENTATION: u32 = 3;
    static WL_DISPLAY_INTERFACE: Interface = Interface {
        name: "wl_display",
        version: 1,
        requests: &[],
        events: &[],
        c_ptr: None,
    };
    match find_object(handle, object, &WL_DISPLAY_INTERFACE, 1) {
        Some(display) => handle.post_error(
            display,
            WL_DISPLAY_ERROR_IMPLEMENTATION,
            CString::new(message).unwrap_or_else(|_| c"internal server error".into()),
        ),
        None => log::error!("Failed to resolve wl_display to post an implementation error"),
    }
}

/// Find the object with `protocol_id` belonging to the same client as `owner`.
///
/// `interface` is checked by name; `ANONYMOUS_INTERFACE` matches any interface.
fn find_object(
    handle: &Handle,
    owner: &ObjectId,
    interface: &'static Interface,
    protocol_id: u32,
) -> Option<ObjectId> {
    let client = handle.get_client(owner.clone()).ok()?;
    if !std::ptr::eq(interface, &ANONYMOUS_INTERFACE) {
        return handle
            .object_for_protocol_id(client, interface, protocol_id)
            .ok();
    }
    let mut found = None;
    let _ = handle.with_all_objects_for(client, |id| {
        if id.protocol_id() == protocol_id {
            found = Some(id);
        }
    });
    found
}

/// An object of a protocol loaded at runtime, as seen from C++.
///
/// All methods must be called on the Wayland event-loop thread.
pub struct DynamicResource {
    id: ObjectId,
    version: u32,
    handle: WeakHandle,
    object: Weak<DynamicObject>,
}

impl DynamicResource {
    fn new(handle: &Handle, id: ObjectId, object: &Arc<DynamicObject>) -> Self {
        let version = handle
            .object_info(id.clone())
            .map(|info| info.version)
            .unwrap_or(0);
        DynamicResource {
            id,
            version,
            handle: handle.downgrade(),
            object: Arc::downgrade(object),
        }
    }

    fn handle(&self) -> Result<Handle, Box<dyn Error>> {
        self.handle
            .upgrade()
            .ok_or_else(|| "The Wayland display no longer exists".into())
    }

    /// The protocol id of this object.
    pub fn object_id(&self) -> u32 {
        self.id.protocol_id()
    }

    /// The version of this object.
    pub fn version(&self) -> u32 {
        self.version
    }

    /// The name of this object's interface.
    pub fn interface_name(&self) -> String {
        self.id.interface().name.to_string()
    }

    /// Whether this object still exists.
    pub fn is_alive(&self) -> bool {
        self.handle
            .upgrade()
            .is_some_and(|handle| handle.object_info(self.id.clone()).is_ok())
    }

    /// The client owning this object.
    pub fn client(&self) -> Result<Box<WaylandClient>, Box<dyn Error>> {
        let handle = DisplayHandle::from(self.handle()?);
        let client = handle
            .get_client(self.id.clone())
            .map_err(|_| "The object no longer exists")?;
        Ok(Box::new(WaylandClient::new(client, handle)))
    }

    /// Send event `opcode` with `args`.
    ///
    /// # Errors
    /// Returns an error, sending nothing, if the object no longer exists or the
    /// event does not match its descriptor: an unknown opcode, an event newer than
    /// this object's version, or arguments of the wrong number or type, null where
    /// not allowed, or naming objects that do not exist for this client or have
    /// the wrong interface.
    pub fn send_event(&self, opcode: u32, args: &[DynamicArgument]) -> Result<(), Box<dyn Error>> {
        let handle = self.handle()?;
        let interface = self.id.interface();
        let event = u16::try_from(opcode)
            .ok()
            .and_then(|opcode| interface.events.get(usize::from(opcode)))
            .ok_or_else(|| format!("{} has no event with opcode {opcode}", interface.name))?;
        let context = || format!("{}.{}", interface.name, event.name);

        if handle.object_info(self.id.clone()).is_err() {
            return Err(format!("{}: the object no longer exists", context()).into());
        }
        if event.since > self.version {
            return Err(format!(
                "{} requires version {} but the object has version {}",
                context(),
                event.since,
                self.version
            )
            .into());
        }
        if args.len() != event.signature.len() {
            return Err(format!(
                "{} takes {} arguments but {} were given",
                context(),
                event.signature.len(),
                args.len()
            )
            .into());
        }

        let mut arg_interfaces = event.arg_interfaces.iter();
        let mut converted = Vec::with_capacity(args.len());
        for (index, (arg, signature)) in args.iter().zip(event.signature).enumerate() {
            let mismatch = || {
                format!(
                    "{} argument {index} must be {signature:?}, not {:?}",
                    context(),
                    arg.kind
                )
            };
            converted.push(match (signature, arg.kind) {
                (ArgumentType::Int, DynamicArgumentKind::Int) => Argument::Int(arg.int_value),
                (ArgumentType::Uint, DynamicArgumentKind::Uint) => Argument::Uint(arg.uint_value),
                (ArgumentType::Fixed, DynamicArgumentKind::Fixed) => {
                    Argument::Fixed(arg.int_value)
                }
                (ArgumentType::Str(allow_null), DynamicArgumentKind::String) => {
                    if arg.is_null {
                        if *allow_null == AllowNull::No {
                            return Err(format!("{} argument {index} must not be null", context()).into());
                        }
                        Argument::Str(None)
                    } else {
                        let value = CString::new(arg.string_value.as_bytes()).map_err(|_| {
                            format!("{} argument {index} contains a null byte", context())
                        })?;
                        Argument::Str(Some(Box::new(value)))
                    }
                }
                (ArgumentType::Object(allow_null), DynamicArgumentKind::Object) => {
                    let interface = arg_interfaces.next().copied().unwrap_or(&ANONYMOUS_INTERFACE);
                    if arg.object_id == 0 {
                        if *allow_null == AllowNull::No {
                            return Err(format!("{} argument {index} must not be null", context()).into());
                        }
                        Argument::Object(ObjectId::null())
                    } else {
                        Argument::Object(
                            find_object(&handle, &self.id, interface, arg.object_id).ok_or_else(
                                || {
                                    format!(
                                        "{} argument {index}: this client has no {} with id {}",
                                        context(),
                                        interface.name,
                                        arg.object_id
                                    )
                                },
                            )?,
                        )
                    }
                }
                (ArgumentType::NewId, DynamicArgumentKind::NewId) => {
                    let child = event
                        .child_interface
                        .expect("validated at load: new_id has an interface");
                    Argument::NewId(
                        find_object(&handle, &self.id, child, arg.object_id).ok_or_else(|| {
                            format!(
                                "{} argument {index}: this client has no {} with id {} (create it with create_resource)",
                                context(),
                                child.name,
                                arg.object_id
                            )
                        })?,
                    )
                }
                (ArgumentType::Array, DynamicArgumentKind::Array) => {
                    Argument::Array(Box::new(arg.array_value.clone()))
                }
                (ArgumentType::Fd, DynamicArgumentKind::Fd) => {
                    if arg.fd < 0 {
                        return Err(format!("{} argument {index} is not a valid fd", context()).into());
                    }
                    Argument::Fd(arg.fd)
                }
                _ => return Err(mismatch().into()),
            });
        }

        handle
            .send_event(Message {
                sender_id: self.id.clone(),
                opcode: opcode as u16,
                args: converted.into_iter().collect(),
            })
            .map_err(|_| format!("{}: the object no longer exists", context()).into())
    }

    /// Post a protocol error on this object, disconnecting its client.
    pub fn post_error(&self, code: u32, message: &str) {
        if let Some(handle) = self.handle.upgrade() {
            handle.post_error(
                self.id.clone(),
                code,
                CString::new(message).unwrap_or_else(|_| c"protocol error".into()),
            );
        }
    }

    /// Destroy this object without the client asking (e.g. after sending a
    /// destructor event).
    pub fn destroy(&self) {
        if let Some(handle) = self.handle.upgrade() {
            let _ = handle.destroy_object::<ServerState>(&self.id);
        }
    }

    /// Create a server-side object of `interface_name` (from a protocol loaded at
    /// runtime) for this object's client, at this object's version, to be sent as
    /// the `new_id` argument of an event.
    ///
    /// Give the new object a handler with [DynamicResource::set_handler] before
    /// sending it, so that requests the client makes on it can be dispatched.
    pub fn create_resource(
        &self,
        interface_name: &str,
    ) -> Result<Box<DynamicResource>, Box<dyn Error>> {
        let handle = self.handle()?;
        let interface = find_interface(interface_name)
            .ok_or_else(|| format!("Interface \"{interface_name}\" has not been loaded"))?;
        let client = handle
            .get_client(self.id.clone())
            .map_err(|_| "The object no longer exists")?;
        let object = Arc::new(DynamicObject::default());
        let id = handle
            .create_object::<ServerState>(client, interface, self.version, object.clone())
            .map_err(|_| "The client no longer exists")?;
        Ok(Box::new(DynamicResource::new(&handle, id, &object)))
    }

    /// Set the handler that receives this object's requests and destruction.
    pub fn set_handler(
        &self,
        handler: SharedPtr<DynamicResourceHandler>,
    ) -> Result<(), Box<dyn Error>> {
        let object = self.object.upgrade().ok_or("The object no longer exists")?;
        object.set_handler(handler);
        Ok(())
    }
}
