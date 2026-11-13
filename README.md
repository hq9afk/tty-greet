# tty-greet

A session picker for the tty. After you log in, it lists your installed Wayland and X11 sessions and starts the one you pick.

## Install

Needs `meson`, `ninja` and `ncurses`.

```sh
./build.sh setup     # install dependencies with pacman
./build.sh install   # build, test, install to /usr
```

Then add `contrib/profile.sh` to `~/.bash_profile` so the picker opens when you log in on ttys 1 to 6.

## Use

- `j`/`k` or the arrow keys move, `Enter` starts the selected session.
- The first entry starts by itself after 5 seconds. Any move key cancels that.
- `Continue in tty` or `q` drops you into the shell.

`tty-greet --list` prints the sessions it found. `tty-greet --preview` shows the picker without starting anything.
