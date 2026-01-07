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
    func: usize, // Function pointer as usize
    data: usize, // Data pointer as usize
}

pub struct EventLoop {
    epoll_fd: RawFd,
    handlers: Arc<Mutex<HashMap<RawFd, FdHandler>>>,
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
            handlers: Arc::new(Mutex::new(HashMap::new())),
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

    pub fn add_fd(&self, fd: i32, mask: u32, func: usize, data: usize) -> bool {
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
            return false;
        }
        
        // Store the handler
        let handler = FdHandler { func, data };
        self.handlers.lock().unwrap().insert(fd, handler);
        true
    }
    
    pub fn remove_fd(&self, fd: i32) {
        let result = unsafe {
            libc::epoll_ctl(
                self.epoll_fd,
                libc::EPOLL_CTL_DEL,
                fd,
                std::ptr::null_mut(),
            )
        };
        
        if result < 0 {
            eprintln!("Failed to remove fd {} from epoll", fd);
            return;
        }
        
        // Remove the handler
        self.handlers.lock().unwrap().remove(&fd);
    }
    
    pub fn dispatch(&self, timeout: i32) -> i32 {
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
        
        let handlers = self.handlers.lock().unwrap();
        
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
            
            if let Some(handler) = handlers.get(&fd) {
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
        ) -> bool;
        
        fn remove_fd(self: &EventLoop, fd: i32);
        
        fn dispatch(self: &EventLoop, timeout: i32) -> i32;
    }
}
