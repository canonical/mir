


#ifndef MIR_SHELL_SESSION_H_
#define MIR_SHELL_SESSION_H_

#include "mir/frontend/session.h"
#include "mir/shell/snapshot.h"

#include <sys/types.h>

namespace mir
{
namespace shell
{
class Surface;

class Session : public frontend::Session
{
public:
    virtual std::string name() const = 0;
    virtual void force_requests_to_complete() = 0;
    virtual pid_t process_id() const = 0;

    virtual void take_snapshot(SnapshotCallback const& snapshot_taken) = 0;
    virtual std::shared_ptr<Surface> default_surface() const = 0;
    virtual void set_lifecycle_state(MirLifecycleState state) = 0;
};
}
}

#endif // MIR_SHELL_SESSION_H_
