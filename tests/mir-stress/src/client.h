// Client state machine class
#include <memory>


/// A simple state machine that knows how to connect to a mir server,
/// issue some commands, and exit cleanly.
class ClientStateMachine
{
public:
    typedef std::shared_ptr<ClientStateMachine> Ptr;
    virtual ~ClientStateMachine() {};

    static Ptr Create();

    /// Connect to the mir server specified by socket_file.
    ///\return True if the connection was successful.
    virtual bool connect(std::string unique_name, const char* socket_file=0) =0;
    virtual bool create_surface() =0;
    virtual bool render_surface() =0;
    virtual void release_surface() =0;
    virtual void disconnect() =0;
};


class MirConnection;
class MirSurface;

class UnacceleratedClient: public ClientStateMachine
{
public:
    UnacceleratedClient();
    bool connect(std::string unique_name, const char* socket_file=0) override;
    bool create_surface() override;
    bool render_surface() override;
    void release_surface() override;
    void disconnect() override;
private:
    MirConnection* connection_;
    MirSurface* surface_;
};
