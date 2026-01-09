mod protocols;

use wayland_server::{Display, DisplayHandle, GlobalDispatch, Dispatch, DataInit, Client, New, Resource};
use std::os::unix::io::{AsRawFd, RawFd};
use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use std::sync::atomic::{AtomicU32, Ordering};
use wayland_server::protocol::wl_compositor::WlCompositor;
use wayland_server::protocol::wl_surface::WlSurface;

struct ServerState {
}

impl ServerState {
    pub fn new() -> Self {
        ServerState {}
    }
}

// impl GlobalDispatch<WlCompositor, ()> for ServerState {
//     fn bind(
//         _state: &mut Self,
//         _handle: &DisplayHandle,
//         _client: &Client,
//         resource: New<WlCompositor>,
//         _global_data: &(),
//         data_init: &mut DataInit<'_, Self>,
//     ) {
//         // From C++, we call wl_global_create with a bind thunk. This should call
//         // into the DisplayHandle to create a new global.
//         //
//         // When a Wayland object is bound, we should call back the bind thunk.
//         //
//         // This bind_think will create the resource for that object with the
//         // provided interface of callbacks using 'wl_resource_create'.

//         // Wait... but this already creates the resource! Yes, because it
//         // sets up the Dispatch impl internally.
//         //
//         // So we will need to update the C++ generator.

//         // Initialize the resource when a client binds to this global
//         data_init.init(resource, ());
//     }
// }

// impl Dispatch<WlCompositor, ()> for ServerState {
//     fn request(
//         _state: &mut Self,
//         _client: &Client,
//         _resource: &WlCompositor,
//         request: <WlCompositor as Resource>::Request,
//         _data: &(),
//         _dhandle: &DisplayHandle,
//         _data_init: &mut DataInit<'_, Self>,
//     ) {
//         use wayland_server::{
//             protocol::wl_compositor::{WlCompositor, Request},
//         };

//         // Handle compositor requests here
//         match request {
//             Request::CreateSurface { id } => {
//                 // Initialize the new surface object
//                 _data_init.init(id, ());
//             }
//             Request::CreateRegion { id } => {
//                 _data_init.init(id, ());
//             }
//             _ => {}
//         }
//     }
// }

// OKIE DOKIE MATT.
//
// Here is the game plan.
//
// So the paradigm for what we want to do here is different, so let's recap the steps:
//
// 1. Generate a Dispatch impl for each protocol object we want to handle (via the existing Wayland generator).
// 2. Match each request on that object (again, this should be generated)
// 3. Use the data field when initing the object to point to a shared_ptr that is shared between Rust and C++
// 4. In the request handler, we can then call methods on that shared_ptr to handle the request

// impl Dispatch<WlSurface, ()> for ServerState {
//     fn request(
//         _state: &mut Self,
//         _client: &Client,
//         _resource: &WlSurface,
//         _request: <WlSurface as Resource>::Request,
//         _data: &(),
//         _dhandle: &DisplayHandle,
//         _data_init: &mut DataInit<'_, Self>,
//     ) {
//         // Handle surface requests 
//         use wayland_server::{
//             protocol::wl_surface::{WlSurface, Request},
//         };
//         match _request {
//             Request::Destroy => {
//                 // Handle surface destruction if needed
//             }
//             Request::Attach { buffer, x, y } => {

//             }
//             _ => {}
//         }
//     }
// }

// TODO: Rename to Server
pub struct DisplayWrapper {
    state: ServerState,
    display: Display<ServerState>,
    eventloop: EventLoop,
    serial: AtomicU32,
    destroy_listeners: Arc<Mutex<HashMap<SourceId, DestroyListener>>>,
    next_listener_id: Arc<Mutex<SourceId>>,
}

impl DisplayWrapper {
    pub fn new() -> Self {
        DisplayWrapper {
            state: ServerState::new(),
            display: Display::new().expect("Failed to create wayland display"),
            eventloop: EventLoop::new(),
            serial: AtomicU32::new(0),
            destroy_listeners: Arc::new(Mutex::new(HashMap::new())),
            next_listener_id: Arc::new(Mutex::new(1)),
        }
    }

    pub fn terminate(self: Box<Self>) {
    }
    
    pub fn dispatch_pending(&mut self) -> i32 {
        let mut state = &mut self.state;
        match self.display.dispatch_clients(&mut state) {
            Ok(_) => 0,
            Err(_) => -1,
        }
    }
    
    pub fn flush_clients(&mut self) -> i32 {
        match self.display.flush_clients() {
            Ok(_) => 0,
            Err(_) => -1,
        }
    }
    
    pub fn get_eventloop(&mut self) -> &mut EventLoop {
        &mut self.eventloop
    }
    
    pub fn next_serial(&self) -> u32 {
        self.serial.fetch_add(1, Ordering::SeqCst).wrapping_add(1)
    }
    
    pub fn add_destroy_listener(&self, func: usize, data: usize) -> u64 {
        // Generate unique ID
        let id = {
            let mut next_id = self.next_listener_id.lock().unwrap();
            let id = *next_id;
            *next_id += 1;
            id
        };
        
        let listener = DestroyListener { func, data };
        self.destroy_listeners.lock().unwrap().insert(id, listener);
        id
    }
    
    pub fn remove_destroy_listener(&self, listener_id: u64) -> bool {
        self.destroy_listeners.lock().unwrap().remove(&listener_id).is_some()
    }
    
    pub fn run(&mut self) {
        let poll_fd = self.display.backend().poll_fd();
        let display_fd = poll_fd.as_raw_fd();
        
        loop {
            // Check if terminated
            if *self.eventloop.terminated.lock().unwrap() {
                break;
            }
            
            // Dispatch any pending wayland client events
            if self.dispatch_pending() < 0 {
                break;
            }
            
            // Flush events to clients
            if self.flush_clients() < 0 {
                break;
            }
            
            // Wait for events with a timeout of -1 (infinite)
            const MAX_EVENTS: usize = 32;
            let mut events: [libc::epoll_event; MAX_EVENTS] = unsafe { std::mem::zeroed() };
            
            let nfds = unsafe {
                libc::epoll_wait(
                    self.eventloop.epoll_fd,
                    events.as_mut_ptr(),
                    MAX_EVENTS as i32,
                    -1,
                )
            };
            
            if nfds < 0 {
                break;
            }
            
            // Process events
            let handlers = self.eventloop.fd_handlers.lock().unwrap();
            
            for i in 0..nfds as usize {
                let event = &events[i];
                let fd = event.u64 as i32;
                
                // Check if this is the display fd
                if fd == display_fd {
                    // This is handled by dispatch_pending above, skip it here
                    continue;
                }
                
                let mut mask = 0u32;
                
                // Convert epoll events to mask
                if event.events & libc::EPOLLIN as u32 != 0 {
                    mask |= 1; // WL_EVENT_READABLE
                }
                if event.events & libc::EPOLLOUT as u32 != 0 {
                    mask |= 2; // WL_EVENT_WRITABLE
                }
                if event.events & (libc::EPOLLERR as u32 | libc::EPOLLHUP as u32) != 0 {
                    mask |= 4; // WL_EVENT_ERROR
                }
                
                // Find and call handler
                if let Some(handler) = handlers.values().find(|h| h.fd == fd) {
                    unsafe {
                        let func: unsafe extern "C" fn(i32, u32, *mut std::ffi::c_void) -> i32 
                            = std::mem::transmute(handler.func);
                        let data = handler.data as *mut std::ffi::c_void;
                        func(fd, mask, data);
                    }
                }
            }
        }
    }
}

pub fn create_display_wrapper() -> Box<DisplayWrapper> {
    Box::new(DisplayWrapper::new())
}

struct FdHandler {
    fd: RawFd,
    func: usize, // Function pointer as usize
    data: usize, // Data pointer as usize
}

struct IdleHandler {
    func: usize, // Function pointer as usize
    data: usize, // Data pointer as usize
}

struct DestroyListener {
    func: usize, // Function pointer as usize
    data: usize, // Data pointer as usize
}

type SourceId = u64;

pub struct EventLoop {
    epoll_fd: RawFd,
    next_id: Arc<Mutex<SourceId>>,
    fd_handlers: Arc<Mutex<HashMap<SourceId, FdHandler>>>,
    idle_handlers: Arc<Mutex<HashMap<SourceId, IdleHandler>>>,
    destroy_listeners: Arc<Mutex<HashMap<SourceId, DestroyListener>>>,
    terminated: Arc<Mutex<bool>>,
}

impl EventLoop {
    pub fn new() -> Self {
        let epoll_fd = unsafe {
            libc::epoll_create1(libc::EPOLL_CLOEXEC)
        };
        
        if epoll_fd < 0 {
            panic!("Failed to create epoll instance");
        }
        
        EventLoop {
            epoll_fd,
            next_id: Arc::new(Mutex::new(1)),
            fd_handlers: Arc::new(Mutex::new(HashMap::new())),
            idle_handlers: Arc::new(Mutex::new(HashMap::new())),
            destroy_listeners: Arc::new(Mutex::new(HashMap::new())),
            terminated: Arc::new(Mutex::new(false)),
        }
    }
    
    pub fn add_display_fd(&self, display: &mut DisplayWrapper) {
        let poll_fd = display.display.backend().poll_fd();
        let fd = poll_fd.as_raw_fd();
        
        let mut event = libc::epoll_event {
            events: libc::EPOLLIN as u32,
            u64: fd as u64,
        };
        
        let result = unsafe {
            libc::epoll_ctl(
                self.epoll_fd,
                libc::EPOLL_CTL_ADD,
                fd,
                &mut event as *mut libc::epoll_event,
            )
        };
        
        if result < 0 {
            eprintln!("Failed to add display fd to epoll");
        }
    }

    pub fn add_fd(&self, fd: i32, mask: u32, func: usize, data: usize) -> u64 {
        let mut events = 0u32;
        
        // Convert mask to epoll events
        // Assuming mask uses WL_EVENT_READABLE (1) and WL_EVENT_WRITABLE (2) convention
        if mask & 1 != 0 {
            events |= libc::EPOLLIN as u32;
        }
        if mask & 2 != 0 {
            events |= libc::EPOLLOUT as u32;
        }
        
        let mut event = libc::epoll_event {
            events,
            u64: fd as u64,
        };
        
        let result = unsafe {
            libc::epoll_ctl(
                self.epoll_fd,
                libc::EPOLL_CTL_ADD,
                fd,
                &mut event as *mut libc::epoll_event,
            )
        };
        
        if result < 0 {
            eprintln!("Failed to add fd {} to epoll", fd);
            return 0; // Return 0 as error indicator
        }
        
        // Generate unique ID and store the handler
        let id = {
            let mut next_id = self.next_id.lock().unwrap();
            let id = *next_id;
            *next_id += 1;
            id
        };
        
        let handler = FdHandler { fd, func, data };
        self.fd_handlers.lock().unwrap().insert(id, handler);
        id
    }
    
    pub fn remove_source(&self, source_id: u64) -> bool {
        // Try to remove from fd_handlers first
        if let Some(handler) = self.fd_handlers.lock().unwrap().remove(&source_id) {
            let result = unsafe {
                libc::epoll_ctl(
                    self.epoll_fd,
                    libc::EPOLL_CTL_DEL,
                    handler.fd,
                    std::ptr::null_mut(),
                )
            };
            
            if result < 0 {
                eprintln!("Failed to remove fd {} from epoll", handler.fd);
                return false;
            }
            return true;
        }
        
        // Try to remove from idle_handlers
        if self.idle_handlers.lock().unwrap().remove(&source_id).is_some() {
            return true;
        }
        
        false
    }
    
    pub fn terminate(&self) {
        *self.terminated.lock().unwrap() = true;
    }
    
    pub fn add_idle(&self, func: usize, data: usize) -> u64 {
        // Generate unique ID
        let id = {
            let mut next_id = self.next_id.lock().unwrap();
            let id = *next_id;
            *next_id += 1;
            id
        };
        
        let handler = IdleHandler { func, data };
        self.idle_handlers.lock().unwrap().insert(id, handler);
        id
    }
    
    pub fn dispatch_idle(&self) -> i32 {
        let handlers: Vec<_> = {
            let mut idle_handlers = self.idle_handlers.lock().unwrap();
            idle_handlers.drain().map(|(_, h)| h).collect()
        };
        
        for handler in handlers {
            unsafe {
                let func: unsafe extern "C" fn(*mut std::ffi::c_void) 
                    = std::mem::transmute(handler.func);
                let data = handler.data as *mut std::ffi::c_void;
                func(data);
            }
        }
        
        0
    }
    
    pub fn add_destroy_listener(&self, func: usize, data: usize) -> u64 {
        // Generate unique ID
        let id = {
            let mut next_id = self.next_id.lock().unwrap();
            let id = *next_id;
            *next_id += 1;
            id
        };
        
        let listener = DestroyListener { func, data };
        self.destroy_listeners.lock().unwrap().insert(id, listener);
        id
    }
    
    pub fn remove_destroy_listener(&self, listener_id: u64) -> bool {
        self.destroy_listeners.lock().unwrap().remove(&listener_id).is_some()
    }
    
    pub fn get_fd(&self) -> i32 {
        self.epoll_fd
    }
    
    pub fn dispatch(&self, timeout: i32) -> i32 {
        if *self.terminated.lock().unwrap() {
            return -1;
        }
        
        const MAX_EVENTS: usize = 32;
        let mut events: [libc::epoll_event; MAX_EVENTS] = unsafe { std::mem::zeroed() };
        
        let nfds = unsafe {
            libc::epoll_wait(
                self.epoll_fd,
                events.as_mut_ptr(),
                MAX_EVENTS as i32,
                timeout,
            )
        };
        
        if nfds < 0 {
            return -1;
        }
        
        let handlers = self.fd_handlers.lock().unwrap();
        
        for i in 0..nfds as usize {
            let event = &events[i];
            let fd = event.u64 as i32;
            let mut mask = 0u32;
            
            // Convert epoll events back to mask
            if event.events & libc::EPOLLIN as u32 != 0 {
                mask |= 1; // WL_EVENT_READABLE
            }
            if event.events & libc::EPOLLOUT as u32 != 0 {
                mask |= 2; // WL_EVENT_WRITABLE
            }
            if event.events & (libc::EPOLLERR as u32 | libc::EPOLLHUP as u32) != 0 {
                mask |= 4; // WL_EVENT_ERROR or similar
            }
            
            // Find handler by fd
            if let Some(handler) = handlers.values().find(|h| h.fd == fd) {
                unsafe {
                    let func: unsafe extern "C" fn(i32, u32, *mut std::ffi::c_void) -> i32 
                        = std::mem::transmute(handler.func);
                    let data = handler.data as *mut std::ffi::c_void;
                    func(fd, mask, data);
                }
            }
        }
        
        0
    }
}

impl Drop for DisplayWrapper {
    fn drop(&mut self) {
        // Call all destroy listeners before cleaning up
        let listeners: Vec<_> = {
            let mut destroy_listeners = self.destroy_listeners.lock().unwrap();
            destroy_listeners.drain().map(|(_, l)| l).collect()
        };
        
        for listener in listeners {
            unsafe {
                let func: unsafe extern "C" fn(*mut std::ffi::c_void) 
                    = std::mem::transmute(listener.func);
                let data = listener.data as *mut std::ffi::c_void;
                func(data);
            }
        }
    }
}

impl Drop for EventLoop {
    fn drop(&mut self) {
        // Call all destroy listeners before cleaning up
        let listeners: Vec<_> = {
            let mut destroy_listeners = self.destroy_listeners.lock().unwrap();
            destroy_listeners.drain().map(|(_, l)| l).collect()
        };
        
        for listener in listeners {
            unsafe {
                let func: unsafe extern "C" fn(*mut std::ffi::c_void) 
                    = std::mem::transmute(listener.func);
                let data = listener.data as *mut std::ffi::c_void;
                func(data);
            }
        }
        
        unsafe {
            libc::close(self.epoll_fd);
        }
    }
}

pub fn create_event_loop() -> Box<EventLoop> {
    Box::new(EventLoop::new())
}

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        type DisplayWrapper;
        type EventLoop;
        
        fn create_display_wrapper() -> Box<DisplayWrapper>;
        
        fn dispatch_pending(self: &mut DisplayWrapper) -> i32;
        
        fn flush_clients(self: &mut DisplayWrapper) -> i32;
        
        fn get_eventloop(self: &mut DisplayWrapper) -> &mut EventLoop;
        
        fn run(self: &mut DisplayWrapper);
        
        fn next_serial(self: &DisplayWrapper) -> u32;
        
        unsafe fn add_destroy_listener(self: &DisplayWrapper, func: usize, data: usize) -> u64;
        
        fn remove_destroy_listener(self: &DisplayWrapper, listener_id: u64) -> bool;

        fn create_event_loop() -> Box<EventLoop>;
        
        fn add_display_fd(self: &EventLoop, display: &mut DisplayWrapper);
        
        unsafe fn add_fd(
            self: &EventLoop, 
            fd: i32, 
            mask: u32, 
            func: usize,
            data: usize
        ) -> u64;
        
        fn remove_source(self: &EventLoop, source_id: u64) -> bool;
        
        fn terminate(self: &EventLoop);
        
        unsafe fn add_idle(self: &EventLoop, func: usize, data: usize) -> u64;
        
        fn dispatch_idle(self: &EventLoop) -> i32;
        
        unsafe fn add_destroy_listener(self: &EventLoop, func: usize, data: usize) -> u64;
        
        fn remove_destroy_listener(self: &EventLoop, listener_id: u64) -> bool;
        
        fn get_fd(self: &EventLoop) -> i32;
        
        fn dispatch(self: &EventLoop, timeout: i32) -> i32;
    }
}
