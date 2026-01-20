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

use calloop::{generic::Generic, EventLoop, Interest, Mode, PostAction};
use std::os::unix::io::AsRawFd;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::time::Duration;
use wayland_server::{
    backend::{ClientData, ClientId, DisconnectReason},
    Display, ListeningSocket,
};

/// The wayland server.
pub struct WaylandServer {
    /// The display.
    display: Display<ServerState>,

    /// The global state for the server.
    state: ServerState,

    /// Stop flag
    stop_requested: Arc<AtomicBool>,
}

impl WaylandServer {
    /// Create a new wayland server.
    pub fn new() -> Self {
        WaylandServer {
            display: Display::new().expect("Failed to create Wayland display"),
            state: ServerState {},
            stop_requested: Arc::new(AtomicBool::new(false)),
        }
    }

    /// Run the wayland server.
    ///
    /// This function will block the current thread.
    ///
    /// # Arguments
    /// * `socket` - The name of the socket to bind to (e.g. "wayland-0").
    pub fn run(&mut self, socket: &str) {
        let mut event_loop: EventLoop<'_, WaylandServer> =
            EventLoop::try_new().expect("Failed to create event loop");
        let loop_handle = event_loop.handle();

        // First, add the listener to the event loop.
        let listener = ListeningSocket::bind(socket).expect("Failed to bind Wayland socket");
        loop_handle
            .insert_source(
                Generic::new(listener, Interest::READ, Mode::Level),
                |_, listener, server: &mut WaylandServer| {
                    if let Ok(stream) = listener.accept() {
                        if let Some(stream) = stream {
                            // Insert the client into the display.
                            // This registers the client's socket with the Display's internal backend.
                            if let Err(e) = server
                                .display
                                .handle()
                                .insert_client(stream, std::sync::Arc::new(ClientState))
                            {
                                eprintln!("Failed to add client: {}", e);
                            }
                        }
                    }
                    Ok(PostAction::Continue)
                },
            )
            .expect("Failed to insert listener into event loop");

        // Next, get the raw file descriptor from the display's backend and add it to the event loop.
        // We need to get the fd without holding a borrow of the display.
        let display_fd = unsafe {
            let raw_fd = self.display.backend().poll_fd().as_raw_fd();
            std::os::fd::BorrowedFd::borrow_raw(raw_fd)
        };
        loop_handle
            .insert_source(
                Generic::new(display_fd, Interest::READ, Mode::Level),
                |_, _, server: &mut WaylandServer| {
                    // This function processes requests from all connected clients
                    match server.display.dispatch_clients(&mut server.state) {
                        Ok(_) => Ok(PostAction::Continue),
                        Err(e) => {
                            eprintln!("Dispatch error: {}", e);
                            Ok(PostAction::Continue)
                        }
                    }
                },
            )
            .expect("Failed to insert backend source into event loop");

        loop {
            if self.stop_requested.load(Ordering::Acquire) {
                break;
            }

            // 1. Dispatch events
            // The event loop borrows `server` temporarily to run the callbacks
            event_loop
                .dispatch(Some(Duration::from_millis(16)), self)
                .expect("Failed to dispatch event loop");

            // 2. Flush clients
            // Because `event_loop.dispatch` only borrowed `server`, we get it back here.
            self.display
                .flush_clients()
                .expect("Failed to flush clients");
        }

        self.stop_requested.store(false, Ordering::SeqCst);
    }

    /// Stops the wayland server if it is running.
    pub fn stop(&mut self) {
        self.stop_requested.store(true, Ordering::Release);
    }
}

/// Create a new wayland server.
pub fn create_wayland_server() -> Box<WaylandServer> {
    Box::new(WaylandServer::new())
}

/// The state of the wayland server.
struct ServerState;

/// The state of a wayland client.
struct ClientState;

impl ClientData for ClientState {
    fn initialized(&self, _client_id: ClientId) {}

    fn disconnected(&self, _client_id: ClientId, _reason: DisconnectReason) {}
}
