#pragma once

#include <string>
#include <string_view>
#include <vector>

enum class SessionType { Wayland,
                         X11 };

struct Session {
    std::string name;
    std::string exec;
    std::string desktop_names;
    std::string file_id; // e.g. `hyprland.desktop`
    SessionType type = SessionType::Wayland;
};

// Keys of the `[Desktop Entry]` group that matter for a session.
struct DesktopEntry {
    std::string name;
    std::string exec;
    std::string try_exec;
    std::string desktop_names;
    bool hidden = false;
    bool no_display = false;
};

// `wayland` or `x11`.
std::string_view type_name(SessionType type);

// Parse `.desktop` file text; only the `[Desktop Entry]` group is read.
DesktopEntry parse_desktop(std::string_view text);

// Absolute path of `prog` (via `$PATH` unless it contains `/`), or empty if not executable.
std::string find_in_path(std::string_view prog);

// `$XDG_DATA_DIRS`, or `/usr/local/share:/usr/share` when unset.
std::vector<std::string> data_dirs();

/*
Sessions under `<dir>/wayland-sessions` and `<dir>/xsessions`, earlier dirs shadowing later ones.
Sorted Wayland first, then by name.
*/
std::vector<Session> discover(const std::vector<std::string> &dirs);
