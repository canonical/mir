use wayland_server::{backend::ClientId, Client, DisplayHandle};

/// A C++ friendly wrapper around a wayland [Client] object.
pub struct WaylandClient {
    client: Client,
    handle: DisplayHandle,
}

impl WaylandClient {
    pub fn new(client: Client, handle: DisplayHandle) -> Self {
        WaylandClient { client, handle }
    }

    /// Retrieve the pid of the client.
    pub fn pid(&self) -> i32 {
        self.client
            .get_credentials(&self.handle)
            .map_or(0, |c| c.pid)
    }

    /// Retrieve the uid of the client.
    pub fn uid(&self) -> u32 {
        self.client
            .get_credentials(&self.handle)
            .map_or(0, |c| c.uid)
    }

    /// Retrieve the gid of the client.
    pub fn gid(&self) -> u32 {
        self.client
            .get_credentials(&self.handle)
            .map_or(0, |c| c.gid)
    }

    /// Check if this client is wrapping the provided [WaylandClientId].
    pub fn equals(&self, id: &WaylandClientId) -> bool {
        self.client.id() == id.id
    }
}

/// An opaque ID for the WaylandClient.
///
/// This is useful when a client is removed because we cannot resolve
/// a WaylandClient form a raw [ClientId]. Instead, the C++ side of things
/// can use [WaylandClient::equals] to check if this id matches one of their
/// clients.
pub struct WaylandClientId {
    id: ClientId,
}

impl WaylandClientId {
    pub fn new(id: ClientId) -> WaylandClientId {
        WaylandClientId { id }
    }
}
