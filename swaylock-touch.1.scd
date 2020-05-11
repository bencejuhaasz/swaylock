swaylock-touch(1)

# NAME

swaylock-touch - Touch pinpad-based screen locker for Wayland based on Drew DeVault's swaylock


# SYNOPSIS

_swaylock-touch_ [options...]

Locks your Wayland session.

# OPTIONS

*-C, --config* <path>
	The config file to use. By default, the following paths are checked:
	_$HOME/.swaylock/config_, _$XDG\_CONFIG\_HOME/swaylock/config_, and
	_SYSCONFDIR/swaylock/config_. All flags aside from this one are valid
	options in the configuration file using the format _long-option=value_.
	For options such as _ignore-empty-password_, just supply the _long-option_.
	All leading dashes should be omitted and the equals sign is required for
	flags that take an argument.

*-d, --debug*
	Enable debugging output.

*-e, --ignore-empty-password*
	When an empty password is provided, do not validate it.

*-F, --show-failed-attempts*
	Show current count of failed authentication attempts.

*-f, --daemonize*
	Detach from the controlling terminal after locking.

	Note: this is the default behavior of i3lock.

*-h, --help*
	Show help message and quit.

*-v, --version*
	Show the version number and quit.

# APPEARANCE

*--button-background-color* <rrggbb[aa]>
	Button background color.

*--button-background-pressed-color* <rrggbb[aa]>
	Button background color when pressed.

*--button-border-color* <rrggbb[aa]>
	Button border color.

*--button-text-color* <rrggbb[aa]>
	Button text color.

*-i, --image* [[<output>]:]<path>
	Display the given image, optionally only on the given output. Use -c to set
	a background color. If the path potentially contains a ':', prefix it with another
	':' to prevent interpreting part of it as <output>.

*-s, --scaling*
	Image scaling mode: _stretch_, _fill_, _fit_, _center_, _tile_,
	_solid\_color_. Use _solid\_color_ to display only the background color, even
	if a background image is specified.

*-t, --tiling*
	Same as --scaling=tile.

*-c, --color* <rrggbb[aa]>
	Turn the screen into the given color instead of white. If -i is used, this
	sets the background of the image to the given color. Defaults to white
	(FFFFFF).

*--font* <font>
	Sets the font of the text.

*--font-size* <size>
	Sets a fixed font size for the indicator text.

*--line-color* <rrggbb[aa]>
	Sets the color of the line between the inside and ring.

*--line-ver-color* <rrggbb[aa]>
	Sets the color of the line between the inside and ring when verifying.

*--line-wrong-color* <rrggbb[aa]>
	Sets the color of the line between the inside and ring when invalid.

*--separator-color* <rrggbb[aa]>
	Sets the color of the lines that separate highlight segments.

*--text-color* <rrggbb[aa]>
	Sets the color of the text.

*--text-ver-color* <rrggbb[aa]>
	Sets the color of the text when verifying.

*--text-wrong-color* <rrggbb[aa]>
	Sets the color of the text when invalid.

# AUTHORS

Based on Drew DeVault's <sir@cmpwn.com> and other open source contributors'. Swaylock-touch maintained by gg-rewrite. For more information about swaylock development, see
https://github.com/swaywm/swaylock.
