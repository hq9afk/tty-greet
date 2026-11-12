#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "session.h"

// `${XDG_STATE_HOME:-$HOME/.local/state}/tui-greet/last`, or empty if neither is set.
std::string state_path();

// Stable id of a session, e.g. `wayland/hyprland.desktop`.
std::string session_key(const Session &s);

// Saved key for the "continue in tty" entry.
inline constexpr std::string_view kTtyKey = "tty";

// The saved key, or empty if unreadable.
std::string load_last(const std::string &path);

// Best effort: failures are ignored.
void save_last(const std::string &path, const std::string &key);

// Index of the session with `key`, `sessions.size()` for `kTtyKey`, or 0 when absent.
std::size_t find_index(const std::vector<Session> &sessions, std::string_view key);
