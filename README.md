# tui-greet

A terminal session picker for after a tty login. You log in on a tty as usual, `tui-greet` lists the installed Wayland and X11 sessions, and starting one replaces the login shell, so logging out of the session ends the tty login.

## Build

`./build.sh` uses Meson and Ninja and needs `ncursesw`.

- `./build.sh setup`: install dependencies with `pacman`.
- `./build.sh`: build and run the unit tests (same as `./build.sh test`).
- `./build.sh build`: build only.
- `./build.sh run --list`: print the sessions that were found, which works inside a desktop too.
- `./build.sh run --preview`: open the real picker without starting anything, which works inside a desktop too. `Enter` does nothing, `q` or `Esc` quits, and the last choice is not changed.
- `./build.sh install` / `./build.sh uninstall`: install to or remove from `/usr`.

`TUI_GREET_BUILD_JOBS=N` sets the number of parallel compile jobs.

## Setup

After `./build.sh install`, add the snippet from `contrib/profile.sh` (installed to `/usr/share/tui-greet/profile.sh`) to `~/.zprofile` or `~/.bash_profile`. It starts `tui-greet` only on ttys 1 to 6 when no graphical session is running.

## Use

The date and time, as `Mon 2026-11-12 14:03:27`, sit below the session list in the accent color of `astralia-clock`. They are hidden when the terminal is too small to fit both.

- The first entry starts by itself after 5 seconds, with the countdown shown on the hint line. Any move key cancels it for the rest of the run. In `--preview` the countdown is shown but does nothing when it runs out.
- `j`/`k` or the arrow keys move and wrap around at either end, `g`/`G` or `Home`/`End` jump to the first or last entry.
- `Enter` starts the selected session.
- The last entry, `Continue in tty`, skips graphics and leaves you in the shell on the current tty.
- `q` or `Esc` also quits to the shell, but without remembering the choice.

The picker always opens on the first entry. The last choice, including `Continue in tty`, is still written to `${XDG_STATE_HOME:-~/.local/state}/tui-greet/last` but is not preselected.

## How sessions are found and started

Sessions are read from `wayland-sessions/` and `xsessions/` under each directory in `$XDG_DATA_DIRS` (default `/usr/local/share:/usr/share`). Earlier directories shadow later ones with the same file name. Entries with `Hidden=true`, `NoDisplay=true`, or a `TryExec` program that is not installed are skipped.

Before starting a session, `tui-greet` sets `XDG_SESSION_TYPE`, `XDG_SESSION_DESKTOP`, and `XDG_CURRENT_DESKTOP` (from `DesktopNames`). Wayland sessions run their `Exec` command directly. X11 sessions run through `startx` with the full path of the `Exec` program.
