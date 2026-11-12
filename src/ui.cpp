#include "ui.h"

#include <curses.h>
#include <time.h>

#include <algorithm>
#include <clocale>
#include <cwchar>
#include <string>

#include "clock.h"

namespace {

constexpr short kPairFrame = 1;
constexpr short kPairTag = 2;
constexpr short kPairClock = 3;
constexpr int kDateGap = 1;   // blank rows between the box and the date
constexpr int kMinInner = 48; // minimum box width inside the frame, shrunk to fit narrow terminals
constexpr struct {
    int r, g, b;
} kAccent{0x9B, 0x57, 0xF4}; // `astralia-clock` accent
constexpr int kTagWidth = 9; // `[wayland]`
const std::wstring kTitle = L" tui-greet ";
const std::wstring kHint = L"j/k move  enter start  q shell";
const std::wstring kTtyName = L"Continue in tty";

std::wstring widen(const std::string &s) {
    std::wstring out(s.size(), L'\0');
    std::mbstate_t st{};
    const char *src = s.c_str();
    const std::size_t n = std::mbsrtowcs(out.data(), &src, out.size(), &st);
    if (n == static_cast<std::size_t>(-1))
        return std::wstring(s.begin(), s.end());
    out.resize(n);
    return out;
}

int cell_width(wchar_t c) {
    const int w = wcwidth(c);
    return w < 0 ? 1 : w;
}

int width(const std::wstring &s) {
    int w = 0;
    for (wchar_t c : s)
        w += cell_width(c);
    return w;
}

// Draw `s` at `y, x`, clipped to `cols` cells.
void put(int y, int x, const std::wstring &s, int cols) {
    int used = 0;
    std::size_t n = 0;
    for (; n < s.size(); ++n) {
        const int w = cell_width(s[n]);
        if (used + w > cols)
            break;
        used += w;
    }
    if (n > 0)
        mvaddnwstr(y, x, s.c_str(), static_cast<int>(n));
}

void frame(int y, int x, int h, int w) {
    attron(COLOR_PAIR(kPairFrame));
    mvaddch(y, x, ACS_ULCORNER);
    mvhline(y, x + 1, ACS_HLINE, w - 2);
    mvaddch(y, x + w - 1, ACS_URCORNER);
    mvvline(y + 1, x, ACS_VLINE, h - 2);
    mvvline(y + 1, x + w - 1, ACS_VLINE, h - 2);
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    mvhline(y + h - 1, x + 1, ACS_HLINE, w - 2);
    mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
    attroff(COLOR_PAIR(kPairFrame));
}

// Pair `kPairClock` = `kAccent`: exact if colors are redefinable, else nearest 256-color. Same as `astralia-clock`.
bool init_accent() {
    if (COLORS < 256)
        return false; // keep default color
    constexpr short id = 16;
    constexpr auto c = kAccent;
    auto scaled = [](int v, int max) { return static_cast<short>((v * max + 127) / 255); }; // 0..255 -> 0..max
    short fg = id;
    if (can_change_color())
        init_color(id, scaled(c.r, 1000), scaled(c.g, 1000), scaled(c.b, 1000));
    else
        fg = static_cast<short>(16 + 36 * scaled(c.r, 5) + 6 * scaled(c.g, 5) + scaled(c.b, 5));
    init_pair(kPairClock, fg, -1);
    return true;
}

bool accent_ready = false;

// The date line, centered on `cols` at row `y`.
void draw_date(int y, int cols, const std::string &date) {
    const std::wstring d = widen(date);
    attron(A_BOLD);
    if (accent_ready)
        attron(COLOR_PAIR(kPairClock));
    put(y, std::max(0, (cols - width(d)) / 2), d, cols);
    if (accent_ready)
        attroff(COLOR_PAIR(kPairClock));
    attroff(A_BOLD);
}

// Milliseconds on the monotonic clock.
long mono_ms() {
    timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);
    return static_cast<long>(t.tv_sec) * 1000 + t.tv_nsec / 1000000;
}

// `left > 0`: the countdown hint replaces the key hint.
void draw(const std::vector<Session> &sessions, const std::vector<std::wstring> &names, std::size_t sel, std::size_t &top,
          std::time_t now, int left) {
    erase();
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);
    if (rows < 6 || cols < 24) {
        put(0, 0, L"terminal too small", cols);
        refresh();
        return;
    }

    int name_w = 0;
    for (const std::wstring &n : names)
        name_w = std::max(name_w, width(n));
    const int want = std::max({1 + name_w + 2 + kTagWidth + 1, width(kHint) + 2, width(kTitle) + 4, kMinInner});
    const int inner = std::min(want, cols - 2);
    const std::size_t n = names.size(); // sessions plus the tty entry
    const int date_h = kDateGap + 1;    // gap, date line
    // A blank row above the list when the whole list, the padding and the date all fit.
    const int pad = rows >= static_cast<int>(n) + 5 + date_h ? 1 : 0;
    const int list_h = static_cast<int>(std::min<std::size_t>(n, rows - 4 - pad));
    const int box_h = list_h + 4 + pad; // borders, padding, list, blank, hint
    const int box_w = inner + 2;

    // The date sits below the box only when both fit; otherwise the box alone is centered.
    const bool show_date = rows >= box_h + date_h;
    const int block_h = box_h + (show_date ? date_h : 0);
    const int y0 = (rows - block_h) / 2;
    const int x0 = (cols - box_w) / 2;
    if (show_date)
        draw_date(y0 + box_h + kDateGap, cols, format_date(now).data());

    frame(y0, x0, box_h, box_w);
    attron(A_BOLD);
    put(y0, x0 + 2, kTitle, inner - 2);
    attroff(A_BOLD);

    top = scroll_top(sel, top, static_cast<std::size_t>(list_h));
    for (int i = 0; i < list_h; ++i) {
        const std::size_t idx = top + static_cast<std::size_t>(i);
        const int y = y0 + 1 + pad + i;
        const bool chosen = idx == sel;
        if (chosen)
            attron(A_REVERSE);
        mvhline(y, x0 + 1, ' ', inner);
        put(y, x0 + 2, names[idx], inner - kTagWidth - 3);
        const std::wstring tag = idx == sessions.size()                       ? L"[tty]"
                                 : sessions[idx].type == SessionType::Wayland ? L"[wayland]"
                                                                              : L"[x11]";
        if (!chosen)
            attron(COLOR_PAIR(kPairTag));
        put(y, x0 + inner - width(tag), tag, width(tag));
        if (!chosen)
            attroff(COLOR_PAIR(kPairTag));
        if (chosen)
            attroff(A_REVERSE);
    }
    if (top > 0)
        mvaddch(y0, x0 + inner - 1, '^');
    if (top + static_cast<std::size_t>(list_h) < n)
        mvaddch(y0 + box_h - 1, x0 + inner - 1, 'v');

    const std::wstring hint =
        left > 0 ? L"starting " + names[0] + L" in " + std::to_wstring(left) + L"s  move to cancel" : kHint;
    attron(A_DIM);
    put(y0 + box_h - 2, x0 + 1 + std::max(0, (inner - width(hint)) / 2), hint, inner);
    attroff(A_DIM);
    refresh();
}

} // namespace

std::optional<std::size_t> pick_session(const std::vector<Session> &sessions, std::size_t initial, bool preview) {
    std::setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    set_escdelay(25);
    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(kPairFrame, COLOR_CYAN, -1);
        init_pair(kPairTag, COLOR_MAGENTA, -1);
        accent_ready = init_accent();
    }

    std::vector<std::wstring> names;
    for (const Session &s : sessions)
        names.push_back(widen(s.name));
    names.push_back(kTtyName);

    const std::size_t n = names.size();
    std::size_t sel = std::min(initial, n - 1);
    std::size_t top = 0;
    std::optional<std::size_t> result;
    const long start_ms = mono_ms();
    bool counting = true; // cleared for good by any move key
    for (bool done = false; !done;) {
        const long elapsed_ms = mono_ms() - start_ms;
        const int left = counting ? countdown_left(elapsed_ms, kCountdownSecs) : 0;
        if (counting && left == 0) {
            counting = false;
            if (!preview) {
                result = 0;
                break;
            }
        }
        // One clock read: `time()` is tick-coarse and lags the second boundary.
        timespec now{};
        clock_gettime(CLOCK_REALTIME, &now);
        draw(sessions, names, sel, top, now.tv_sec, left);
        int wait_ms = static_cast<int>(1000 - now.tv_nsec / 1000000); // wake at the next second
        if (left > 0) {
            const int to_tick = static_cast<int>(1000 - (elapsed_ms % 1000)); // next countdown second
            wait_ms = std::min(wait_ms, to_tick);
        }
        timeout(wait_ms);
        const int ch = getch(); // ERR on timeout: redraw the date and countdown
        if (ch == ERR || ch == KEY_RESIZE)
            continue;
        switch (ch) {
        case KEY_UP:
        case 'k':
            counting = false;
            sel = sel > 0 ? sel - 1 : n - 1; // wrap to the last entry
            break;
        case KEY_DOWN:
        case 'j':
            counting = false;
            sel = sel + 1 < n ? sel + 1 : 0; // wrap to the first entry
            break;
        case KEY_HOME:
        case 'g':
            counting = false;
            sel = 0;
            break;
        case KEY_END:
        case 'G':
            counting = false;
            sel = n - 1;
            break;
        case '\n':
        case '\r':
        case KEY_ENTER:
            if (preview)
                break;
            result = sel;
            done = true;
            break;
        case 'q':
        case 27: // Esc
            done = true;
            break;
        default:
            break;
        }
    }
    endwin();
    return result;
}
