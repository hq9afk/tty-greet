#include <cassert>
#include <string>
#include <vector>

#include "clock.h"
#include "launch.h"
#include "session.h"
#include "ui.h"

using Args = std::vector<std::string>;

namespace {

bool has_env(const LaunchPlan &p, const std::string &key, const std::string &value) {
    for (const auto &[k, v] : p.env)
        if (k == key && v == value)
            return true;
    return false;
}

bool has_key(const LaunchPlan &p, const std::string &key) {
    for (const auto &kv : p.env)
        if (kv.first == key)
            return true;
    return false;
}

} // namespace

int main(int argc, char **argv) {
    assert(argc == 2);
    const std::string fixtures = argv[1];

    // Exec tokenizing: quotes, escapes, field codes.
    assert(tokenize_exec("Hyprland") == Args({"Hyprland"}));
    assert(tokenize_exec("  uwsm start  -e %U ") == Args({"uwsm", "start", "-e"}));
    assert(tokenize_exec(R"(sh -c "exec \"a b\" \\x \$y")") == Args({"sh", "-c", R"(exec "a b" \x $y)"}));
    assert(tokenize_exec(R"(app "" 100%% %f)") == Args({"app", "", "100%"}));
    assert(tokenize_exec("").empty());
    assert(tokenize_exec("%F").empty());

    // `.desktop` parsing: only `[Desktop Entry]`, no localized keys, string escapes.
    const DesktopEntry e = parse_desktop(
        "# c\n[Desktop Entry]\nName = Foo\\sBar\nName[fr]=Le Foo\nExec=foo\nHidden=true\n[Desktop Action x]\nExec=bar\n");
    assert(e.name == "Foo Bar");
    assert(e.exec == "foo");
    assert(e.hidden && !e.no_display);
    assert(parse_desktop("Exec=outside\n").exec.empty());

    // `$PATH` lookup.
    assert(find_in_path("sh").front() == '/');
    assert(find_in_path("/nonexistent/x").empty());
    assert(find_in_path("definitely-not-a-program-tty-greet").empty());

    // Discovery: shadowing, hidden/NoDisplay/TryExec filtering, Wayland first, case-insensitive order.
    const std::vector<Session> s = discover({fixtures + "/a", fixtures + "/b", fixtures + "/none"});
    assert(s.size() == 4);
    assert(s[0].name == "Alpha" && s[0].type == SessionType::Wayland && s[0].file_id == "alpha.desktop");
    assert(s[1].name == "beta");
    assert(s[2].name == "Dup A");
    assert(s[3].name == "X Test" && s[3].type == SessionType::X11);

    // Launch plans.
    const auto wl = plan_launch(s[0]);
    assert(wl && wl->argv == Args({"sh", "-c", "exec alpha"}));
    assert(has_env(*wl, "XDG_SESSION_TYPE", "wayland"));
    assert(has_env(*wl, "XDG_CURRENT_DESKTOP", "Alpha:Wlroots"));
    assert(has_env(*wl, "XDG_SESSION_DESKTOP", "alpha"));
    assert(!has_key(*plan_launch(s[1]), "XDG_CURRENT_DESKTOP"));

    const auto x = plan_launch(s[3]);
    assert(x && x->argv.size() == 4 && x->argv[0] == "startx" && x->argv[1].front() == '/');
    assert(x->argv[2] == "-c" && x->argv[3] == "true");
    assert(has_env(*x, "XDG_SESSION_TYPE", "x11"));

    Session gone = s[3];
    gone.exec = "definitely-not-a-program-tty-greet";
    assert(!plan_launch(gone));
    gone.exec = "%U";
    assert(!plan_launch(gone));

    // Date.
    const std::time_t t = 1791000306; // 2026-10-03 04:05:06 UTC, a Saturday
    assert(std::string(format_date(t, true).data()) == "Sat 2026-10-03 04:05:06");

    // Scrolling.
    assert(scroll_top(0, 0, 5) == 0);
    assert(scroll_top(4, 0, 5) == 0);
    assert(scroll_top(5, 0, 5) == 1);
    assert(scroll_top(2, 3, 5) == 2);
    assert(scroll_top(9, 3, 5) == 5);

    // Countdown: whole seconds left, rounded up.
    assert(countdown_left(0, 5) == 5);
    assert(countdown_left(1, 5) == 5);
    assert(countdown_left(999, 5) == 5);
    assert(countdown_left(1000, 5) == 4);
    assert(countdown_left(4999, 5) == 1);
    assert(countdown_left(5000, 5) == 0);
    assert(countdown_left(9000, 5) == 0);
}
