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

use calloop::ping::Ping;
use calloop::{generic::Generic, EventLoop, Interest, Mode, PostAction};
use log;
use std::error;
use std::option::Option;
use std::sync::Mutex;
use wayland_server::{
    backend::{ClientData, ClientId, DisconnectReason},
    Display, DisplayHandle, ListeningSocket,
};

/// The wayland server.
pub struct WaylandServer {
    stop_signal: Mutex<Option<Ping>>,
}

impl WaylandServer {
    /// Create a new wayland server.
    pub fn new() -> Self {
        WaylandServer {
            stop_signal: Mutex::new(None),
        }
    }

    /// Run the wayland server.
    ///
    /// This function will block the current thread.
    ///
    /// # Arguments
    /// * `socket` - The name of the socket to bind to (e.g. "wayland-0").
    pub fn run(&mut self, socket: &str) -> Result<(), Box<dyn error::Error>> {
        let display = Display::<ServerState>::new()?;
        let mut event_loop: EventLoop<'_, ServerState> = EventLoop::try_new()?;
        let loop_handle = event_loop.handle();
        let mut state = ServerState {
            handle: display.handle(),
            stop_requested: false,
        };

        // First, add the listener to the event loop.
        let listener = ListeningSocket::bind(socket)?;
        loop_handle.insert_source(
            Generic::new(listener, Interest::READ, Mode::Level),
            |_, listener, state: &mut ServerState| {
                if let Ok(stream) = listener.accept() {
                    if let Some(stream) = stream {
                        // Insert the client into the display.
                        // This registers the client's socket with the Display's internal backend.
                        if let Err(e) = state
                            .handle
                            .insert_client(stream, std::sync::Arc::new(ClientState))
                        {
                            log::error!("Failed to add client: {}", e);
                        }
                    }
                }
                Ok(PostAction::Continue)
            },
        )?;

        loop_handle.insert_source(
            Generic::new(display, Interest::READ, Mode::Level),
            |_, server, state: &mut ServerState| {
                // This function processes requests from all connected clients

                // SAFETY:
                // NoIoDrop<Display> exposes `get_mut` as unsafe because we could use a mutable
                // reference to drop or close the underlying IO object.
                // We are not doing that; this upholds the required invariant
                unsafe {
                    if let Err(e) = server.get_mut().dispatch_clients(state) {
                        log::error!("Error dispatching wayland clients: {}", e);
                    }
                }
                Ok(PostAction::Continue)
            },
        )?;

        // Add stop_signal to wake and terminate the loop.
        let (pinger, ping_source) = calloop::ping::make_ping()?;

        *self
            .stop_signal
            .lock()
            .expect("No recovery from lock poisioning") = Some(pinger);

        loop_handle
            .insert_source(ping_source, |_, _, server: &mut ServerState| {
                server.stop_requested = true;
            })
            .map_err(|_| "Failed to insert stop eventfd into event loop")?;

        while !state.stop_requested {
            // 1. Dispatch events
            // The event loop borrows `server` temporarily to run the callbacks
            event_loop
                .dispatch(None, &mut state)
                .map_err(|_| "Failed to dispatch event loop")?;

            // 2. Flush clients
            // Because `event_loop.dispatch` only borrowed `server`, we get it back here.
            state
                .handle
                .flush_clients()
                .map_err(|_| "Failed to flush clients")?;
        }

        Ok(())
    }

    /// Stops the wayland server if it is running.
    pub fn stop(&self) {
        if let Some(signal) = self
            .stop_signal
            .lock()
            .expect("No way to recover from lock poisioning")
            .as_ref()
        {
            signal.ping();
        }
    }
}

/// Create a new wayland server.
pub fn create_wayland_server() -> Box<WaylandServer> {
    Box::new(WaylandServer::new())
}

/// The state of the wayland server.
struct ServerState {
    handle: DisplayHandle,
    stop_requested: bool,
}

/// The state of a wayland client.
struct ClientState;

impl ClientData for ClientState {
    fn initialized(&self, _client_id: ClientId) {}

    fn disconnected(&self, _client_id: ClientId, _reason: DisconnectReason) {}
}
