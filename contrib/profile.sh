# tui-greet: add to ~/.zprofile or ~/.bash_profile.
# Runs only on a real tty (1-6) with no graphical session yet.
# No `exec`: quitting the picker leaves you in the shell; a started session replaces it.
if [ -z "$DISPLAY" ] && [ -z "$WAYLAND_DISPLAY" ] && [ -n "$XDG_VTNR" ] && [ "$XDG_VTNR" -le 6 ] && command -v tui-greet >/dev/null; then
    tui-greet
fi
