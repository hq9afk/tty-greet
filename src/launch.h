#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "session.h"

struct LaunchPlan {
    std::vector<std::string> argv;
    std::vector<std::pair<std::string, std::string>> env;
};

// Split an `Exec=` value per the Desktop Entry spec, dropping `%` field codes.
std::vector<std::string> tokenize_exec(std::string_view exec);

// The argv and environment to start `s`; `nullopt` if its command is empty or, for X11, not found.
std::optional<LaunchPlan> plan_launch(const Session &s);

// Replace this process with `s`. Returns only on failure, after printing why; the result is the exit code.
int launch(const Session &s);
