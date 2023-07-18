//
// Created by matthew on 7/18/23.
//

#ifndef MIR_APPLICATION_SELECTOR_H
#define MIR_APPLICATION_SELECTOR_H

#include <miral/window_manager_tools.h>
#include <miral/application.h>
#include <functional>

namespace miral
{

/// Manages the selection of applications using keyboard shortcuts,
/// most likely via Alt + Tab. While "start" is called, the next application
/// in the SessionContainer's list after the currently focus application
/// will be raised. When "next" is called, the application after that will be
/// raised, and so on. When "stop" is called, the currently raised application
/// will gain focus and be returned.
class ApplicationSelector
{
public:
    ApplicationSelector(WindowManagerTools const&);

    /// Begins application selection by raising the application
    /// that immediately follows the currently selected application
    /// in the SessionContainer's list.
    void start();

    /// Lowers the currently raised application and raises the application
    /// that follows it.
    /// \returns The raised application
    auto next() -> Application ;

    /// Focuses the currently selected Application.
    /// \returns The newly selected application
    auto stop() -> Application ;

private:
    WindowManagerTools tools;
    bool is_active = false;
};

}


#endif //MIR_APPLICATION_SELECTOR_H
