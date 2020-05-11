# swaylock-touch

swaylock-touch is a touchscreen pinpad screen locking utility for Wayland compositor based on swaywm/swaylock. It is compatible
with any Wayland compositor which implements the following Wayland protocols:

- wlr-layer-shell
- wlr-input-inhibitor
- xdg-output
- xdg-shell

See the man page, `swaylock-touch(1)`, for instructions on using swaylock.

## Release Signatures

Releases are signed with [B22DA89A](http://pgp.mit.edu/pks/lookup?op=vindex&search=0x52CB6609B22DA89A)
and published [on GitHub](https://github.com/swaywm/swaylock/releases). swaylock
releases are managed independently of sway releases.

## Installation

### Compiling from Source

Install dependencies:

* meson \*
* wayland
* wayland-protocols \*
* libxkbcommon
* cairo
* gdk-pixbuf2 \*\*
* pam (optional)
* [scdoc](https://git.sr.ht/~sircmpwn/scdoc) (optional: man pages) \*
* git \*

_\*Compile-time dep_

_\*\*optional: required for background images other than PNG_

Run these commands:

    meson build
    ninja -C build
    sudo ninja -C build install

On systems without PAM, you need to suid the swaylock-touch binary:

    sudo chmod a+s /usr/local/bin/swaylock-touch

Swaylock-touch will drop root permissions shortly after startup.

## Regarding passwords

Pinpad-based screen locker only accepts numerical passwords. Either set your user password to an all-numeric one or use PAM's separate authorization for the program (e.g. via pam_pwdfile)
