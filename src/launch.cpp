#include "launch.h"

#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

std::vector<std::string> tokenize_exec(std::string_view exec) {
    std::vector<std::string> out;
    std::string cur;
    bool have = false; // a token has started (an empty `""` still counts)
    bool quoted = false;
    for (std::size_t i = 0; i < exec.size(); ++i) {
        const char c = exec[i];
        if (quoted) {
            if (c == '"') {
                quoted = false;
            } else if (c == '\\' && i + 1 < exec.size() && std::strchr("\"`$\\", exec[i + 1])) {
                cur += exec[++i];
            } else if (c == '%' && i + 1 < exec.size()) {
                if (exec[++i] == '%')
                    cur += '%';
            } else {
                cur += c;
            }
            continue;
        }
        if (c == ' ' || c == '\t') {
            if (have)
                out.push_back(std::move(cur));
            cur.clear();
            have = false;
        } else if (c == '"') {
            quoted = true;
            have = true;
        } else if (c == '%' && i + 1 < exec.size()) {
            if (exec[++i] == '%') {
                cur += '%';
                have = true;
            }
        } else {
            cur += c;
            have = true;
        }
    }
    if (have)
        out.push_back(std::move(cur));
    return out;
}

std::optional<LaunchPlan> plan_launch(const Session &s) {
    std::vector<std::string> args = tokenize_exec(s.exec);
    if (args.empty())
        return std::nullopt;

    LaunchPlan plan;
    if (s.type == SessionType::X11) {
        // `startx` reads a non-absolute first argument as server options, so pass the full path.
        std::string client = find_in_path(args[0]);
        if (client.empty())
            return std::nullopt;
        args[0] = std::move(client);
        plan.argv.emplace_back("startx");
    }
    plan.argv.insert(plan.argv.end(), args.begin(), args.end());

    plan.env.emplace_back("XDG_SESSION_TYPE", std::string(type_name(s.type)));
    std::string desktops = s.desktop_names;
    for (char &c : desktops)
        if (c == ';')
            c = ':';
    while (!desktops.empty() && desktops.back() == ':')
        desktops.pop_back();
    if (!desktops.empty())
        plan.env.emplace_back("XDG_CURRENT_DESKTOP", desktops);
    std::string stem = s.file_id;
    if (stem.size() > 8 && stem.compare(stem.size() - 8, 8, ".desktop") == 0)
        stem.resize(stem.size() - 8);
    plan.env.emplace_back("XDG_SESSION_DESKTOP", stem);
    return plan;
}

int launch(const Session &s) {
    const std::optional<LaunchPlan> plan = plan_launch(s);
    if (!plan) {
        std::fprintf(stderr, "tty-greet: cannot start %s: command not found: %s\n", s.name.c_str(), s.exec.c_str());
        return 1;
    }
    for (const auto &[key, value] : plan->env)
        setenv(key.c_str(), value.c_str(), 1);

    std::vector<char *> argv;
    for (const std::string &a : plan->argv)
        argv.push_back(const_cast<char *>(a.c_str()));
    argv.push_back(nullptr);
    execvp(argv[0], argv.data());

    std::fprintf(stderr, "tty-greet: cannot start %s: %s: %s\n", s.name.c_str(), argv[0], std::strerror(errno));
    return 1;
}
