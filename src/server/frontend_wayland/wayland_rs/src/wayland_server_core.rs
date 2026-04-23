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

use crate::ffi::{FdReadyCallback, GlobalFactory, WaylandServerNotificationHandler, WorkCallback};
use crate::wayland_client::{WaylandClient, WaylandClientId};
use calloop::channel::{self, Event};
use calloop::ping::Ping;
use calloop::{generic::Generic, EventLoop, Interest, Mode, PostAction, RegistrationToken};
use cxx::UniquePtr;
use log;
use std::error;
use std::option::Option;
use std::os::fd::{AsRawFd, FromRawFd, OwnedFd};
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::{mpsc, Arc, Mutex};
use wayland_server::{
    backend::{ClientData, ClientId, DisconnectReason},
    Display, DisplayHandle, ListeningSocket,
};

// SAFETY: WorkCallback instances are created on one thread and sent to the event loop
// thread via calloop::channel. The C++ implementation must be safe to call from the
// event loop thread, which matches the existing WaylandExecutor contract.
unsafe impl Send for WorkCallback {}

// SAFETY: FdReadyCallback instances are created on one thread and moved into a calloop
// source callback. They are only ever called from the event loop thread.
unsafe impl Send for FdReadyCallback {}

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

        // Set up the work channel for cross-thread work queueing.
        let (work_sender, work_channel) = channel::channel::<UniquePtr<WorkCallback>>();
        loop_handle
            .insert_source(work_channel, |event, _, _state: &mut ServerState| {
                if let Event::Msg(mut work) = event {
                    work.pin_mut().execute();
                }
            })
            .map_err(|_| "Failed to insert work channel into event loop")?;

        // Create the event loop handle and notify C++ that the loop is ready.
        let event_loop_handle = WaylandEventLoopHandle {
            loop_handle: loop_handle.clone(),
            work_sender,
        };
        state
            .notification_handler
            .pin_mut()
            .loop_ready(Box::new(event_loop_handle));

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

/// A handle to the Wayland event loop that allows C++ code to watch file descriptors
/// and queue arbitrary work onto the loop thread.
///
/// This is safe to use from any thread: the `work_sender` is `Send + Sync`.
pub struct WaylandEventLoopHandle {
    loop_handle: calloop::LoopHandle<'static, ServerState>,
    work_sender: channel::Sender<UniquePtr<WorkCallback>>,
}

impl WaylandEventLoopHandle {
    /// Register a file descriptor for one-shot readable notification.
    ///
    /// When the fd becomes readable, `callback.ready()` is called on the event loop thread
    /// and the watch is automatically removed.
    ///
    /// The returned `FdWatchToken` can be used to cancel the watch early. Dropping the token
    /// also cancels the watch.
    ///
    /// # Safety
    /// The caller must ensure `fd` is a valid, open file descriptor. Ownership of the fd
    /// is transferred to this function — it will be closed when the watch is removed.
    pub fn watch_fd(&self, fd: i32, callback: UniquePtr<FdReadyCallback>) -> Box<FdWatchToken> {
        // SAFETY: caller guarantees fd is valid and transfers ownership
        let owned_fd = unsafe { OwnedFd::from_raw_fd(fd) };
        let callback = Arc::new(Mutex::new(Some(callback)));
        let callback_clone = callback.clone();
        let token = self
            .loop_handle
            .insert_source(
                Generic::new(owned_fd, Interest::READ, Mode::Level),
                move |_, _, _| {
                    if let Some(mut cb) = callback_clone
                        .lock()
                        .expect("No recovery from lock poisoning")
                        .take()
                    {
                        cb.pin_mut().ready();
                    }
                    Ok(PostAction::Remove)
                },
            )
            .expect("Failed to insert fd source into event loop");
        Box::new(FdWatchToken {
            token: Some(token),
            loop_handle: self.loop_handle.clone(),
        })
    }

    /// Queue work to be executed on the Wayland event loop thread.
    ///
    /// This is safe to call from any thread. The work will be dispatched
    /// during the next event loop iteration.
    pub fn spawn(&self, work: UniquePtr<WorkCallback>) {
        if let Err(e) = self.work_sender.send(work) {
            log::error!("Failed to send work to event loop: {}", e);
        }
    }

    /// Clone this handle so it can be shared with other C++ components.
    pub fn clone_box(&self) -> Box<WaylandEventLoopHandle> {
        Box::new(WaylandEventLoopHandle {
            loop_handle: self.loop_handle.clone(),
            work_sender: self.work_sender.clone(),
        })
    }
}

/// A token representing a registered fd watch. Dropping or calling `cancel()` removes the watch.
pub struct FdWatchToken {
    token: Option<RegistrationToken>,
    loop_handle: calloop::LoopHandle<'static, ServerState>,
}

impl FdWatchToken {
    /// Cancel the fd watch, removing it from the event loop.
    /// Safe to call multiple times — subsequent calls are no-ops.
    pub fn cancel(&mut self) {
        if let Some(token) = self.token.take() {
            self.loop_handle.remove(token);
        }
    }
}

impl Drop for FdWatchToken {
    fn drop(&mut self) {
        self.cancel();
    }
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
