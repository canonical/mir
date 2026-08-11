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
use calloop::ping::Ping;
use calloop::{
    generic::{FdWrapper, Generic},
    EventLoop, Interest, LoopHandle, Mode, PostAction, RegistrationToken,
};
use cxx::UniquePtr;
use log;
use std::error;
use std::option::Option;
use std::os::fd::{AsRawFd, FromRawFd, RawFd};
use std::os::unix::net::UnixStream;
use std::sync::{mpsc, Arc, Mutex};
use wayland_server::{
    backend::{ClientData, ClientId, DisconnectReason},
    Display, DisplayHandle, ListeningSocket,
};

/// A registration of interest in a file descriptor becoming readable, awaiting
/// insertion into the running event loop.
type PendingFdListener = (RawFd, Box<dyn FdReadyListener>);

/// The wayland server.
pub struct WaylandServer {
    stop_signal: Mutex<Option<Ping>>,
    work_signal: Mutex<Option<Ping>>,
    fd_listener_signal: Mutex<Option<Ping>>,
    pending_fd_listeners: Mutex<Vec<PendingFdListener>>,
    client_signal: Mutex<Option<Ping>>,
    pending_clients: Mutex<Vec<RawFd>>,
    /// A clone of the running display's handle, populated for the duration of
    /// [WaylandServer::run]. It lets C++ create and destroy dynamic globals
    /// (e.g. one `wl_output` per monitor) from any thread; the handle is
    /// internally synchronised by the backend.
    display_handle: Mutex<Option<DisplayHandle>>,
}

impl WaylandServer {
    /// Create a new wayland server.
    pub fn new() -> Self {
        WaylandServer {
            stop_signal: Mutex::new(None),
            work_signal: Mutex::new(None),
            fd_listener_signal: Mutex::new(None),
            pending_fd_listeners: Mutex::new(Vec::new()),
            client_signal: Mutex::new(None),
            pending_clients: Mutex::new(Vec::new()),
            display_handle: Mutex::new(None),
        }
    }

    /// Create a `wl_output` global backed by the given per-output binder.
    ///
    /// Unlike the statically-registered globals, `wl_output` globals are
    /// dynamic: Mir advertises one per connected monitor and withdraws it when
    /// the monitor is unplugged. The `binder` captures the C++ state for a
    /// single monitor and is invoked whenever a client binds this global.
    ///
    /// The returned [OutputGlobal] owns the global's lifetime: dropping it
    /// (from C++, via the returned `Box`) removes the global from the display.
    ///
    /// # Panics
    /// Panics if called while the server is not running (i.e. outside of an
    /// active [WaylandServer::run]).
    pub fn create_output_global(
        &self,
        binder: UniquePtr<crate::ffi::OutputGlobalBinder>,
    ) -> Box<OutputGlobal> {
        let handle = self
            .display_handle
            .lock()
            .expect("No recovery from lock poisoning")
            .as_ref()
            .expect("create_output_global called while the server was not running")
            .clone();

        let data = crate::dispatch::OutputGlobalData::new(binder);

        let version =
            <wayland_server::protocol::wl_output::WlOutput as wayland_server::Resource>::interface(
            )
            .version;

        let id = handle.create_global::<
            ServerState,
            wayland_server::protocol::wl_output::WlOutput,
            crate::dispatch::OutputGlobalData,
        >(version, data);

        Box::new(OutputGlobal { id, handle })
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
        work_callback: UniquePtr<WorkCallback>,
    ) -> Result<(), Box<dyn error::Error>> {
        let display = Display::<ServerState>::new()?;
        let mut event_loop: EventLoop<'_, ServerState> = EventLoop::try_new()?;
        let loop_handle = event_loop.handle();
        let (disconnect_tx, disconnect_rx) = mpsc::channel::<(ClientId, DisconnectReason)>();
        let mut state = ServerState {
            handle: display.handle(),
            stop_requested: false,
            notification_handler,
            work_callback,
            disconnect_rx,
        };

        // Publish a clone of the display handle so that C++ can create and
        // destroy dynamic globals (e.g. per-monitor `wl_output`s) while the
        // server is running. Cleared again before `run` returns.
        *self
            .display_handle
            .lock()
            .expect("No recovery from lock poisoning") = Some(state.handle.clone());

        // First, add the listener to the event loop.
        let listener = ListeningSocket::bind(socket)?;
        let listener_disconnect_tx = disconnect_tx.clone();
        loop_handle.insert_source(
            Generic::new(listener, Interest::READ, Mode::Level),
            move |_, listener, state: &mut ServerState| {
                if let Ok(stream) = listener.accept() {
                    if let Some(stream) = stream {
                        // Insert the client into the display. This registers the
                        // client's socket with the Display's internal backend.
                        WaylandServer::insert_stream_client(state, &listener_disconnect_tx, stream);
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
        let (stop_pinger, stop_ping_source) = calloop::ping::make_ping()?;

        *self
            .stop_signal
            .lock()
            .expect("No recovery from lock poisoning") = Some(stop_pinger);

        loop_handle
            .insert_source(stop_ping_source, |_, _, server: &mut ServerState| {
                server.stop_requested = true;
            })
            .map_err(|_| "Failed to insert stop eventfd into event loop")?;

        // Add a work signal so that C++ can ask (from any thread) for its
        // pending work to be drained on the event loop thread. The ping
        // coalesces multiple requests into a single wake; when it fires we ask
        // the C++ side to run all the work it currently has queued.
        let (work_pinger, work_ping_source) = calloop::ping::make_ping()?;

        *self
            .work_signal
            .lock()
            .expect("No recovery from lock poisoning") = Some(work_pinger);

        loop_handle
            .insert_source(work_ping_source, |_, _, state: &mut ServerState| {
                state.work_callback.pin_mut().execute();
            })
            .map_err(|_| "Failed to insert work signal into event loop")?;

        // Add an fd-listener signal so that C++ can ask (from any thread) for the
        // file descriptors it registered to be inserted into the event loop. The
        // ping coalesces multiple requests into a single wake; when it fires we
        // drain the queue of pending registrations and insert a source for each.
        let (fd_listener_pinger, fd_listener_ping_source) = calloop::ping::make_ping()?;

        *self
            .fd_listener_signal
            .lock()
            .expect("No recovery from lock poisoning") = Some(fd_listener_pinger);

        let fd_listener_loop_handle = loop_handle.clone();
        let fd_listener_queue = &self.pending_fd_listeners;
        loop_handle
            .insert_source(fd_listener_ping_source, move |_, _, _: &mut ServerState| {
                WaylandServer::drain_pending_fd_listeners(
                    &fd_listener_loop_handle,
                    fd_listener_queue,
                );
            })
            .map_err(|_| "Failed to insert fd listener signal into event loop")?;

        // Add a client-injection signal so that C++ can ask (from any thread) for
        // a pre-connected socket fd to be adopted as a Wayland client (used for
        // MirAL internal clients and the WLCS test harness). The ping coalesces
        // multiple requests into a single wake; when it fires we drain the queue
        // of pending fds and insert a client for each.
        let (client_pinger, client_ping_source) = calloop::ping::make_ping()?;

        *self
            .client_signal
            .lock()
            .expect("No recovery from lock poisoning") = Some(client_pinger);

        let client_disconnect_tx = disconnect_tx.clone();
        let client_queue = &self.pending_clients;
        loop_handle
            .insert_source(client_ping_source, move |_, _, state: &mut ServerState| {
                WaylandServer::drain_pending_clients(client_queue, &client_disconnect_tx, state);
            })
            .map_err(|_| "Failed to insert client signal into event loop")?;

        WaylandServer::register_globals(&state, Arc::new(Mutex::new(factory)));

        // Drain any work the C++ side queued before the work signal was ready.
        // Such a signal would have been dropped (the queue is owned by C++ and
        // outlives `run`), so run that work now that the loop is starting. Work
        // queued from here on raises its own signal and is handled by the loop.
        state.work_callback.pin_mut().execute();

        // Likewise, register any fd listeners queued before the fd-listener
        // signal was ready; their signal would have been dropped. Registrations
        // queued from here on raise their own signal and are handled by the loop.
        Self::drain_pending_fd_listeners(&loop_handle, &self.pending_fd_listeners);

        // Likewise, adopt any clients injected before the client signal was
        // ready; their signal would have been dropped. Injections queued from
        // here on raise their own signal and are handled by the loop.
        Self::drain_pending_clients(&self.pending_clients, &disconnect_tx, &mut state);

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

        // The display handle is about to become inert; stop handing out clones
        // for dynamic global creation.
        *self
            .display_handle
            .lock()
            .expect("No recovery from lock poisoning") = None;

        Ok(())
    }

    /// Stops the wayland server if it is running.
    pub fn stop(&self) {
        if let Some(signal) = self
            .stop_signal
            .lock()
            .expect("No way to recover from lock poisoning")
            .as_ref()
        {
            signal.ping();
        }
    }

    /// Signal the event loop to drain any work the C++ side has queued.
    ///
    /// The work itself is owned by the C++ side. When the event loop processes
    /// this signal it calls back into C++ via `WorkCallback::execute()`, on the
    /// event-loop thread, which runs all of the C++ work currently pending.
    ///
    /// This may be called from any thread. Signals coalesce, so the C++ side
    /// need only call this while it has no signal already outstanding.
    ///
    /// Returns `true` if the signal was delivered, or `false` if the server is
    /// not currently running (i.e. `run` has not been called yet) and the
    /// signal was dropped. The caller is responsible for re-signalling later in
    /// the latter case.
    pub fn drain_queue(&self) -> bool {
        match self
            .work_signal
            .lock()
            .expect("No way to recover from lock poisoning")
            .as_ref()
        {
            Some(pinger) => {
                pinger.ping();
                true
            }
            None => false,
        }
    }

    /// Register interest in a file descriptor becoming ready for reading.
    ///
    /// This is a **one-shot** registration: when `fd` first becomes readable the
    /// event loop calls back into C++ via `FdReadyCallback::ready()`, on the
    /// event-loop thread, and then stops watching the descriptor (the listener
    /// is dropped). To be notified again, register the descriptor afresh from
    /// within (or after) `ready()`.
    ///
    /// The descriptor is only borrowed; ownership (and closing) remains the
    /// responsibility of the C++ caller, which must keep it valid until the
    /// notification fires (or the registration is otherwise dropped).
    ///
    /// This may be called from any thread, and either before or while the
    /// server is running. Registrations queued before the server starts are
    /// applied when `run` begins; registrations made while it is running wake
    /// the loop so they are applied promptly.
    pub fn register_fd_ready_listener(&self, fd: i32, callback: UniquePtr<FdReadyCallback>) {
        self.enqueue_fd_listener(fd, Box::new(CxxFdReadyListener(callback)));
    }

    /// Queue an fd listener and, if the server is running, wake the loop so the
    /// registration is applied. Split out from `register_fd_ready_listener` so
    /// the queueing behaviour can be exercised without a C++ callback.
    fn enqueue_fd_listener(&self, fd: RawFd, listener: Box<dyn FdReadyListener>) {
        self.pending_fd_listeners
            .lock()
            .expect("No recovery from lock poisoning")
            .push((fd, listener));

        if let Some(signal) = self
            .fd_listener_signal
            .lock()
            .expect("No recovery from lock poisoning")
            .as_ref()
        {
            signal.ping();
        }
    }

    /// Insert a one-shot source into `handle` that invokes `listener` the first
    /// time `fd` becomes readable, then removes itself from the loop.
    ///
    /// The descriptor is borrowed for the lifetime of the registration; calloop
    /// never closes it.
    fn register_fd_ready_source<'l, S>(
        handle: &LoopHandle<'l, S>,
        fd: RawFd,
        mut listener: Box<dyn FdReadyListener>,
    ) -> Result<RegistrationToken, Box<dyn error::Error>> {
        if fd < 0 {
            return Err("Invalid file descriptor".into());
        }

        // SAFETY: The fd is owned by the C++ caller, which is responsible for
        // keeping it valid until the notification fires. `FdWrapper` only
        // borrows the descriptor.
        let source = Generic::new(unsafe { FdWrapper::new(fd) }, Interest::READ, Mode::Level);
        handle
            .insert_source(source, move |_, _, _: &mut S| {
                listener.ready();
                // One-shot: stop watching the descriptor after the first
                // notification. This drops the source (and the listener).
                Ok(PostAction::Remove)
            })
            .map_err(|_| "Failed to insert fd ready source into event loop".into())
    }

    /// Drain `queue` of pending fd registrations, inserting a source for each
    /// into `handle`. Runs on the event-loop thread.
    fn drain_pending_fd_listeners<'l, S>(
        handle: &LoopHandle<'l, S>,
        queue: &Mutex<Vec<PendingFdListener>>,
    ) {
        let pending: Vec<PendingFdListener> = {
            let mut queue = queue.lock().expect("No recovery from lock poisoning");
            std::mem::take(&mut *queue)
        };

        for (fd, listener) in pending {
            if let Err(e) = Self::register_fd_ready_source(handle, fd, listener) {
                log::error!("Failed to register fd ready listener: {}", e);
            }
        }
    }

    /// Adopt a pre-connected socket `fd` as a Wayland client.
    ///
    /// This is how Mir injects clients that connect over a socket pair rather
    /// than the listening socket: MirAL internal clients (see
    /// `Server::open_client_wayland`) and the WLCS test harness (see
    /// `Server::open_wayland_client_socket`). The server end of the pair is
    /// handed here; ownership of `fd` is transferred to the server, which closes
    /// it when the client disconnects.
    ///
    /// This may be called from any thread, and either before or while the server
    /// is running. Injections queued before the server starts are applied when
    /// `run` begins; injections made while it is running wake the loop so they
    /// are applied promptly. Injections coalesce onto a single wake.
    pub fn insert_client(&self, fd: i32) {
        self.pending_clients
            .lock()
            .expect("No recovery from lock poisoning")
            .push(fd as RawFd);

        if let Some(signal) = self
            .client_signal
            .lock()
            .expect("No recovery from lock poisoning")
            .as_ref()
        {
            signal.ping();
        }
    }

    /// Adopt `stream` as a Wayland client: register it with the display's
    /// backend and notify C++ of the new client. Shared by the listening-socket
    /// accept path and the injected-fd path so both behave identically. Runs on
    /// the event-loop thread.
    fn insert_stream_client(
        state: &mut ServerState,
        disconnect_tx: &mpsc::Sender<(ClientId, DisconnectReason)>,
        stream: UnixStream,
    ) {
        let disconnect_tx = disconnect_tx.clone();
        let client_state = ClientState {
            on_disconnect: Box::new(move |client_id, reason| {
                log::info!("Client disconnected: {:?}, reason: {:?}", client_id, reason);
                // We publish the notification about the disconnected client onto
                // the channel so that the WaylandServerNotificationHandler is
                // guaranteed to only be spoken to on a single thread.
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
                let wayland_client = WaylandClient::new(client.clone(), state.handle.clone());
                state
                    .notification_handler
                    .pin_mut()
                    .client_added(Box::new(wayland_client));
            }
        }
    }

    /// Drain `queue` of pending injected client fds, adopting a client for each.
    /// Runs on the event-loop thread.
    fn drain_pending_clients(
        queue: &Mutex<Vec<RawFd>>,
        disconnect_tx: &mpsc::Sender<(ClientId, DisconnectReason)>,
        state: &mut ServerState,
    ) {
        let pending: Vec<RawFd> = {
            let mut queue = queue.lock().expect("No recovery from lock poisoning");
            std::mem::take(&mut *queue)
        };

        for fd in pending {
            // SAFETY: `fd` is the server end of a socket pair whose ownership was
            // transferred to us by the C++ caller via `insert_client`; nothing
            // else touches it, so we may take ownership here.
            let stream = unsafe { UnixStream::from_raw_fd(fd) };
            Self::insert_stream_client(state, disconnect_tx, stream);
        }
    }
}

/// Notified when a registered file descriptor becomes ready for reading.
pub trait FdReadyListener: Send {
    /// Called on the event-loop thread the first time the descriptor is
    /// readable. The registration is one-shot, so this fires at most once.
    fn ready(&mut self);
}

/// Adapts the C++ `FdReadyCallback` to the internal `FdReadyListener` trait.
struct CxxFdReadyListener(UniquePtr<FdReadyCallback>);

// SAFETY: The wrapped C++ callback is moved onto the pending-listener queue and
// from there to the event-loop thread, where it is exclusively used and
// eventually dropped. It is never shared, and our `FdReadyCallback`
// implementations have no thread affinity, so transferring ownership across
// threads is sound.
unsafe impl Send for CxxFdReadyListener {}

impl FdReadyListener for CxxFdReadyListener {
    fn ready(&mut self) {
        if let Some(callback) = self.0.as_mut() {
            callback.ready();
        }
    }
}

/// Create a new wayland server.
pub fn create_wayland_server() -> Box<WaylandServer> {
    Box::new(WaylandServer::new())
}

/// An RAII handle owning the lifetime of a dynamically-created `wl_output`
/// global (see [WaylandServer::create_output_global]).
///
/// Dropping it withdraws the global from the display. It is handed to C++ as a
/// `Box<OutputGlobal>`; when C++ drops that box the global is removed.
pub struct OutputGlobal {
    id: wayland_server::backend::GlobalId,
    handle: DisplayHandle,
}

impl Drop for OutputGlobal {
    fn drop(&mut self) {
        self.handle.remove_global::<ServerState>(self.id.clone());
    }
}

/// The state of the wayland server.
pub struct ServerState {
    pub handle: DisplayHandle,
    stop_requested: bool,
    notification_handler: UniquePtr<WaylandServerNotificationHandler>,
    work_callback: UniquePtr<WorkCallback>,
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
