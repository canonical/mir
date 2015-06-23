#include "mir/test/validity_matchers.h"
#include "mir_toolkit/mir_client_library.h"

template<>
bool IsValidMatcher::MatchAndExplain(MirConnection* connection, MatchResultListener* listener) const
{
    if (connection == nullptr)
    {
        return false;
    }
    if (!mir_connection_is_valid(connection))
    {
        *listener << "error is: " << mir_connection_get_error_message(connection);
        return false;
    }
    return true;
}

template<>
bool IsValidMatcher::MatchAndExplain(MirSurface* surface, MatchResultListener* listener) const
{
    if (surface == nullptr)
    {
        return false;
    }
    if (!mir_surface_is_valid(surface))
    {
        *listener << "error is: " << mir_surface_get_error_message(surface);
        return false;
    }
    return true;
}
