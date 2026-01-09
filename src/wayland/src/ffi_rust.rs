#[cxx::bridge]
mod ffi_rust {
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
