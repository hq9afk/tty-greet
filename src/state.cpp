#include "state.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

std::string state_path() {
    const char *state = std::getenv("XDG_STATE_HOME");
    if (state && *state == '/')
        return std::string(state) + "/tui-greet/last";
    const char *home = std::getenv("HOME");
    if (home && *home)
        return std::string(home) + "/.local/state/tui-greet/last";
    return {};
}

std::string session_key(const Session &s) { return std::string(type_name(s.type)) + '/' + s.file_id; }

std::string load_last(const std::string &path) {
    if (path.empty())
        return {};
    std::ifstream in(path);
    std::string key;
    std::getline(in, key);
    return key;
}

void save_last(const std::string &path, const std::string &key) {
    if (path.empty())
        return;
    std::error_code ec;
    fs::create_directories(fs::path(path).parent_path(), ec);
    std::ofstream out(path, std::ios::trunc);
    out << key << '\n';
}

std::size_t find_index(const std::vector<Session> &sessions, std::string_view key) {
    if (key == kTtyKey)
        return sessions.size();
    for (std::size_t i = 0; i < sessions.size(); ++i)
        if (session_key(sessions[i]) == key)
            return i;
    return 0;
}
