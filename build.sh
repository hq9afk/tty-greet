#!/usr/bin/env bash

# Build script for tui-greet
# Use:
# <empty>: build and run the unit tests
# setup: install dependencies
# build: build only
# test: build and run the unit tests
# install: build and install system-wide
# run: build and start tui-greet, extra arguments are passed to it
# uninstall: remove the system-wide install
# TUI_GREET_BUILD_JOBS=N: parallel compile jobs (default: all cores)

set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"

BIN=tui-greet
JOBS="${TUI_GREET_BUILD_JOBS:-$(nproc)}"

cmd_setup() {
    sudo pacman -Syu --needed gcc meson ninja pkgconf ncurses
}

cmd_build() {
    if [[ ! -f build/build.ninja ]]; then
        meson setup build
    fi
    meson compile -C build -j "$JOBS"
}

cmd_test() { cmd_build; meson test -C build --print-errorlogs; }
cmd_default() { cmd_test; }
cmd_install() { cmd_build; sudo meson install -C build --no-rebuild; }
cmd_run() { cmd_build; "./build/$BIN" "$@"; }
cmd_uninstall() { sudo ninja -C build uninstall; }

main() {
    local cmd="${1:-default}"
    case "$cmd" in
    default | setup | build | test | install | run | uninstall) shift || true; "cmd_$cmd" "$@" ;;
    *) echo "unknown command: $cmd" >&2; exit 2 ;;
    esac
}

main "$@"
