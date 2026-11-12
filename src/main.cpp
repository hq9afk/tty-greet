#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "launch.h"
#include "session.h"
#include "state.h"
#include "ui.h"

namespace {

constexpr const char *kUsage =
    "usage: tui-greet [--list | --preview | --help]\n"
    "\n"
    "Pick a Wayland or X11 session from a list and start it, replacing this process.\n"
    "Meant to run from a login shell profile on a tty.\n"
    "\n"
    "  --list     print the discovered sessions and exit\n"
    "  --preview  show the picker without starting anything (enter does nothing), works inside a desktop\n"
    "  --help     show this help\n"
    "\n"
    "The last entry, \"Continue in tty\", skips graphics and stays in the shell.\n"
    "\n"
    "The first entry starts by itself after 5 seconds; any move key cancels that.\n"
    "\n"
    "keys: j/k or arrows move, enter starts, q or esc quits to the shell\n";

bool set(const char *name) {
    const char *v = std::getenv(name);
    return v && *v;
}

} // namespace

int main(int argc, char **argv) {
    bool list = false;
    bool preview = false;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--help") || !std::strcmp(argv[i], "-h")) {
            std::fputs(kUsage, stdout);
            return 0;
        }
        if (!std::strcmp(argv[i], "--list")) {
            list = true;
            continue;
        }
        if (!std::strcmp(argv[i], "--preview")) {
            preview = true;
            continue;
        }
        std::fprintf(stderr, "tui-greet: unknown argument: %s\n%s", argv[i], kUsage);
        return 2;
    }

    const std::vector<Session> sessions = discover(data_dirs());
    if (list) {
        for (const Session &s : sessions)
            std::printf("%-8s %-28s %-24s %s\n", type_name(s.type).data(), s.file_id.c_str(), s.name.c_str(), s.exec.c_str());
        return 0;
    }

    if (!preview && (set("DISPLAY") || set("WAYLAND_DISPLAY"))) {
        std::fputs("tui-greet: already inside a graphical session\n", stderr);
        return 2;
    }
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        std::fputs("tui-greet: needs a terminal\n", stderr);
        return 2;
    }

    const std::string path = state_path();
    const std::optional<std::size_t> choice = pick_session(sessions, 0, preview);
    if (!choice || preview)
        return 0;
    if (*choice == sessions.size()) {
        save_last(path, std::string(kTtyKey));
        return 0;
    }

    const Session &s = sessions[*choice];
    save_last(path, session_key(s));
    return launch(s);
}
