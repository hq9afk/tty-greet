#include "session.h"

#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <utility>

namespace fs = std::filesystem;

namespace {

std::string_view trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.remove_suffix(1);
    return s;
}

// String-value escapes: `\s \n \t \r \\`.
std::string unescape(std::string_view v) {
    std::string out;
    out.reserve(v.size());
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i] != '\\' || i + 1 == v.size()) {
            out += v[i];
            continue;
        }
        switch (v[++i]) {
        case 's':
            out += ' ';
            break;
        case 'n':
            out += '\n';
            break;
        case 't':
            out += '\t';
            break;
        case 'r':
            out += '\r';
            break;
        case '\\':
            out += '\\';
            break;
        default:
            out += '\\';
            out += v[i];
            break;
        }
    }
    return out;
}

bool is_executable_file(const std::string &path) {
    std::error_code ec;
    return fs::is_regular_file(path, ec) && access(path.c_str(), X_OK) == 0;
}

std::vector<std::string> split(std::string_view s, char sep) {
    std::vector<std::string> out;
    std::size_t start = 0;
    while (start <= s.size()) {
        const std::size_t end = std::min(s.find(sep, start), s.size());
        out.emplace_back(s.substr(start, end - start));
        start = end + 1;
    }
    return out;
}

bool name_less(const Session &a, const Session &b) {
    if (a.type != b.type)
        return a.type == SessionType::Wayland;
    return std::lexicographical_compare(a.name.begin(), a.name.end(), b.name.begin(), b.name.end(), [](char x, char y) {
        return std::tolower(static_cast<unsigned char>(x)) < std::tolower(static_cast<unsigned char>(y));
    });
}

} // namespace

std::string_view type_name(SessionType type) { return type == SessionType::Wayland ? "wayland" : "x11"; }

DesktopEntry parse_desktop(std::string_view text) {
    DesktopEntry e;
    bool in_entry = false;
    for (const std::string &raw : split(text, '\n')) {
        const std::string_view line = trim(raw);
        if (line.empty() || line.front() == '#')
            continue;
        if (line.front() == '[') {
            in_entry = line == "[Desktop Entry]";
            continue;
        }
        if (!in_entry)
            continue;
        const std::size_t eq = line.find('=');
        if (eq == std::string_view::npos)
            continue;
        const std::string_view key = trim(line.substr(0, eq));
        const std::string value = unescape(trim(line.substr(eq + 1)));
        if (key == "Name")
            e.name = value;
        else if (key == "Exec")
            e.exec = value;
        else if (key == "TryExec")
            e.try_exec = value;
        else if (key == "DesktopNames")
            e.desktop_names = value;
        else if (key == "Hidden")
            e.hidden = value == "true";
        else if (key == "NoDisplay")
            e.no_display = value == "true";
    }
    return e;
}

std::string find_in_path(std::string_view prog) {
    if (prog.empty())
        return {};
    if (prog.find('/') != std::string_view::npos) {
        std::string path(prog);
        return is_executable_file(path) ? path : std::string();
    }
    const char *env = std::getenv("PATH");
    for (std::string dir : split(env ? env : "/usr/local/bin:/usr/bin:/bin", ':')) {
        if (dir.empty())
            dir = ".";
        std::string path = dir + '/' + std::string(prog);
        if (!is_executable_file(path))
            continue;
        std::error_code ec;
        const fs::path abs = fs::absolute(path, ec);
        return ec ? path : abs.string();
    }
    return {};
}

std::vector<std::string> data_dirs() {
    const char *env = std::getenv("XDG_DATA_DIRS");
    std::vector<std::string> out;
    for (std::string &dir : split(env && *env ? env : "/usr/local/share:/usr/share", ':'))
        if (!dir.empty())
            out.push_back(std::move(dir));
    return out;
}

std::vector<Session> discover(const std::vector<std::string> &dirs) {
    static const std::pair<const char *, SessionType> kinds[] = {
        {"wayland-sessions", SessionType::Wayland},
        {"xsessions", SessionType::X11},
    };
    std::vector<Session> out;
    std::set<std::pair<SessionType, std::string>> seen;
    for (const std::string &dir : dirs) {
        for (const auto &[sub, type] : kinds) {
            std::vector<fs::path> files;
            std::error_code ec;
            for (auto it = fs::directory_iterator(fs::path(dir) / sub, ec); !ec && it != fs::directory_iterator(); it.increment(ec))
                if (it->path().extension() == ".desktop")
                    files.push_back(it->path());
            std::sort(files.begin(), files.end());

            for (const fs::path &file : files) {
                std::string id = file.filename().string();
                // A shadowed id stays hidden even if the winning file is itself hidden.
                if (!seen.emplace(type, id).second)
                    continue;
                std::ifstream in(file);
                if (!in)
                    continue;
                std::stringstream buf;
                buf << in.rdbuf();
                const DesktopEntry e = parse_desktop(buf.str());
                if (e.hidden || e.no_display || e.exec.empty())
                    continue;
                if (!e.try_exec.empty() && find_in_path(e.try_exec).empty())
                    continue;
                out.push_back({e.name.empty() ? id : e.name, e.exec, e.desktop_names, std::move(id), type});
            }
        }
    }
    std::sort(out.begin(), out.end(), name_less);
    return out;
}
