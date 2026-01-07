use wayland_server::{Display};
use std::os::unix::io::{AsRawFd, RawFd};
use std::collections::HashMap;
use std::sync::{Arc, Mutex};

pub struct DisplayWrapper {
    display: Display<()>,
}

impl DisplayWrapper {
    pub fn terminate(self: Box<Self>) {
    }
}

pub fn create_display_wrapper() -> Box<DisplayWrapper> {
    Box::new(DisplayWrapper {
        display: Display::new().expect("Failed to create wayland display"),
    })
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
