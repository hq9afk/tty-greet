#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "session.h"

// First visible row so that `selected` stays within `visible` rows, moving `top` as little as possible.
inline std::size_t scroll_top(std::size_t selected, std::size_t top, std::size_t visible) {
    if (visible == 0)
        return selected;
    if (selected < top)
        return selected;
    if (selected >= top + visible)
        return selected - visible + 1;
    return top;
}

// Seconds before the first entry starts on its own.
inline constexpr int kCountdownSecs = 5;

// Whole seconds left of a `total_secs` countdown after `elapsed_ms`, rounded up; 0 once it has run out.
inline int countdown_left(long elapsed_ms, int total_secs) {
    const long left_ms = total_secs * 1000L - elapsed_ms;
    return left_ms <= 0 ? 0 : static_cast<int>((left_ms + 999) / 1000);
}

/*
Run the picker. The list ends with a "continue in tty" entry at index `sessions.size()`.
Returns the chosen index, or `nullopt` when quit with `q`/`Esc`. The terminal is restored on return.
A `kCountdownSecs` countdown starts entry 0 when it runs out; any move key cancels it for good.
With `preview`, `Enter` does nothing, so the only way out is `q`/`Esc` and the result is always `nullopt`;
the countdown is still shown but does nothing when it runs out.
*/
std::optional<std::size_t> pick_session(const std::vector<Session> &sessions, bool preview = false);
