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

use crate::ffi::{GlobalFactory, WaylandServerNotificationHandler};
use crate::wayland_client::{WaylandClient, WaylandClientId};
use calloop::ping::Ping;
use calloop::{generic::Generic, EventLoop, Interest, Mode, PostAction};
use cxx::UniquePtr;
use log;
use std::error;
use std::option::Option;
use std::os::fd::AsRawFd;
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::{mpsc, Arc, Mutex};
use wayland_server::{
    backend::{ClientData, ClientId, DisconnectReason},
    Display, DisplayHandle, ListeningSocket,
};

static SERIAL_COUNTER: AtomicU32 = AtomicU32::new(0);

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
    pub fn run(
        &mut self,
        socket: &str,
        factory: UniquePtr<GlobalFactory>,
        notification_handler: UniquePtr<WaylandServerNotificationHandler>,
    ) -> Result<(), Box<dyn error::Error>> {
        let display = Display::<ServerState>::new()?;
        let mut event_loop: EventLoop<'_, ServerState> = EventLoop::try_new()?;
        let loop_handle = event_loop.handle();
        let (disconnect_tx, disconnect_rx) = mpsc::channel::<(ClientId, DisconnectReason)>();
        let mut state = ServerState {
            handle: display.handle(),
            stop_requested: false,
            notification_handler,
            disconnect_rx,
        };

        // First, add the listener to the event loop.
        let listener = ListeningSocket::bind(socket)?;
        loop_handle.insert_source(
            Generic::new(listener, Interest::READ, Mode::Level),
            move |_, listener, state: &mut ServerState| {
                if let Ok(stream) = listener.accept() {
                    if let Some(stream) = stream {
                        // Insert the client into the display.
                        // This registers the client's socket with the Display's internal backend.
                        let disconnect_tx = disconnect_tx.clone();
                        let client_state = ClientState {
                            on_disconnect: Box::new(move |client_id, reason| {
                                log::info!(
                                    "Client disconnected: {:?}, reason: {:?}",
                                    client_id,
                                    reason
                                );
                                // We publish the notification about the disconnected client onto the channel
                                // so that the WaylandServerNotificationHandler is guaranteed to only be
                                // spoken to on a single thread.
                                let _ = disconnect_tx.send((client_id, reason));
                            }),
                            socket_fd: stream.as_raw_fd(),
                        };
                        match state.handle.insert_client(stream, Arc::new(client_state)) {
                            Err(e) => log::error!("Failed to add client: {}", e),
                            Ok(client) => {
                                // Notify C++ that we have a new WaylandClient available to us.
                                // The C++ side of things can choose to hold onto this Box if they
                                // choose to.
                                let wayland_client =
                                    WaylandClient::new(client.clone(), state.handle.clone());
                                state
                                    .notification_handler
                                    .pin_mut()
                                    .client_added(Box::new(wayland_client));
                            }
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

        WaylandServer::register_globals(&state, Arc::new(Mutex::new(factory)));

        // TODO: Don't spin continuously
        while !state.stop_requested {
            // 1. Dispatch events
            // The event loop borrows `server` temporarily to run the callbacks
            event_loop
                .dispatch(None, &mut state)
                .map_err(|_| "Failed to dispatch event loop")?;

            // 2. Process any pending disconnects
            while let Ok((client_id, _reason)) = state.disconnect_rx.try_recv() {
                state
                    .notification_handler
                    .pin_mut()
                    .client_removed(Box::new(WaylandClientId::new(client_id)));
            }

            // 3. Flush clients
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

/// Increment and return the next serial number for the Wayland display.
pub fn next_serial() -> u32 {
    SERIAL_COUNTER.fetch_add(1, Ordering::Relaxed) + 1
}

/// The state of the wayland server.
pub struct ServerState {
    pub handle: DisplayHandle,
    stop_requested: bool,
    notification_handler: UniquePtr<WaylandServerNotificationHandler>,
    disconnect_rx: mpsc::Receiver<(ClientId, DisconnectReason)>,
}

/// The state of a wayland client.
pub struct ClientState {
    on_disconnect: Box<dyn Fn(ClientId, DisconnectReason) + Send + Sync>,
    socket_fd: std::os::fd::RawFd,
}

impl ClientState {
    /// Retrieve the socket file descriptor for this client's connection.
    pub fn fd(&self) -> i32 {
        self.socket_fd
    }
}

impl ClientData for ClientState {
    fn initialized(&self, _client_id: ClientId) {}

    fn disconnected(&self, client_id: ClientId, reason: DisconnectReason) {
        (self.on_disconnect)(client_id, reason);
    }
}
