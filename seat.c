#include <assert.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>
#include <wayland-client.h>
#include "log.h"
#include "swaylock.h"
#include "seat.h"

static void keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard,
			    uint32_t format, int32_t fd, uint32_t size) {
	struct swaylock_state *state = data;
	if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
		close(fd);
		swaylock_log(LOG_ERROR, "Unknown keymap format %d, aborting",
			     format);
		exit(1);
	}
	char *map_shm = mmap(NULL, size - 1, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map_shm == MAP_FAILED) {
		close(fd);
		swaylock_log(LOG_ERROR,
			     "Unable to initialize keymap shm, aborting");
		exit(1);
	}
	struct xkb_keymap *keymap =
		xkb_keymap_new_from_string(state->xkb.context, map_shm,
					   XKB_KEYMAP_FORMAT_TEXT_V1,
					   XKB_KEYMAP_COMPILE_NO_FLAGS);
	munmap(map_shm, size);
	close(fd);
	assert(keymap);
	struct xkb_state *xkb_state = xkb_state_new(keymap);
	assert(xkb_state);
	xkb_keymap_unref(state->xkb.keymap);
	xkb_state_unref(state->xkb.state);
	state->xkb.keymap = keymap;
	state->xkb.state = xkb_state;
}

static void keyboard_enter(void *data, struct wl_keyboard *wl_keyboard,
			   uint32_t serial, struct wl_surface *surface,
			   struct wl_array *keys) {
	// Who cares
}

static void keyboard_leave(void *data, struct wl_keyboard *wl_keyboard,
			   uint32_t serial, struct wl_surface *surface) {
	// Who cares
}

static void keyboard_key(void *data, struct wl_keyboard *wl_keyboard,
			 uint32_t serial, uint32_t time, uint32_t key,
			 uint32_t _key_state) {
	struct swaylock_state *state = data;
	enum wl_keyboard_key_state key_state = _key_state;
	xkb_keysym_t sym = xkb_state_key_get_one_sym(state->xkb.state, key + 8);
	uint32_t keycode =
		key_state == WL_KEYBOARD_KEY_STATE_PRESSED ? key + 8 : 0;
	uint32_t codepoint = xkb_state_key_get_utf32(state->xkb.state, keycode);
	if (keycode != 0) {
	printf("keycode %d codepoint %d\n",
	       keycode, codepoint); }
	if (key_state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		swaylock_handle_key(state, sym, codepoint);
	}
}

static void keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard,
			       uint32_t serial, uint32_t mods_depressed,
			       uint32_t mods_latched, uint32_t mods_locked,
			       uint32_t group) {
  //left blank
}

static void keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
				 int32_t rate, int32_t delay) {
	// TODO
}

static const struct wl_keyboard_listener keyboard_listener = {
	.keymap = keyboard_keymap,
	.enter = keyboard_enter,
	.leave = keyboard_leave,
	.key = keyboard_key,
	.modifiers = keyboard_modifiers,
	.repeat_info = keyboard_repeat_info,
};

static void wl_pointer_enter(void *data, struct wl_pointer *wl_pointer,
			     uint32_t serial, struct wl_surface *surface,
			     wl_fixed_t surface_x, wl_fixed_t surface_y) {
	wl_pointer_set_cursor(wl_pointer, serial, NULL, 0, 0);
}

static void wl_pointer_leave(void *data, struct wl_pointer *wl_pointer,
			     uint32_t serial, struct wl_surface *surface) {
	// Who cares
}

static void wl_pointer_motion(void *data, struct wl_pointer *wl_pointer,
			      uint32_t time, wl_fixed_t surface_x,
			      wl_fixed_t surface_y) {
	// Who cares
}

static void wl_pointer_button(void *data, struct wl_pointer *wl_pointer,
			      uint32_t serial, uint32_t time, uint32_t button,
			      uint32_t state) {
	// Who cares
}

static void wl_pointer_axis(void *data, struct wl_pointer *wl_pointer,
			    uint32_t time, uint32_t axis, wl_fixed_t value) {
	// Who cares
}

static void wl_pointer_frame(void *data, struct wl_pointer *wl_pointer) {
	// Who cares
}

static void wl_pointer_axis_source(void *data, struct wl_pointer *wl_pointer,
				   uint32_t axis_source) {
	// Who cares
}

static void wl_pointer_axis_stop(void *data, struct wl_pointer *wl_pointer,
				 uint32_t time, uint32_t axis) {
	// Who cares
}

static void wl_pointer_axis_discrete(void *data, struct wl_pointer *wl_pointer,
				     uint32_t axis, int32_t discrete) {
	// Who cares
}

static const struct wl_pointer_listener pointer_listener = {
	.enter = wl_pointer_enter,
	.leave = wl_pointer_leave,
	.motion = wl_pointer_motion,
	.button = wl_pointer_button,
	.axis = wl_pointer_axis,
	.frame = wl_pointer_frame,
	.axis_source = wl_pointer_axis_source,
	.axis_stop = wl_pointer_axis_stop,
	.axis_discrete = wl_pointer_axis_discrete,
};

static void wl_touch_down(void *data, struct wl_touch *touch, uint32_t serial,
			  uint32_t time, struct wl_surface *surface, int32_t id,
			  wl_fixed_t x, wl_fixed_t y) {
	struct swaylock_seat *seat = data;
	struct swaylock_state *state = seat->state;
	swaylock_handle_touch(state, TOUCH_EVENT_DOWN, wl_fixed_to_int(x),
			      wl_fixed_to_int(y));
}

static void wl_touch_up(void *data, struct wl_touch *touch, uint32_t serial,
			uint32_t time, int32_t id) {
	struct swaylock_seat *seat = data;
	struct swaylock_state *state = seat->state;
	swaylock_handle_touch(state, TOUCH_EVENT_UP, 0, 0);
}

static void wl_touch_motion(void *data, struct wl_touch *touch, uint32_t time,
			    int32_t id, wl_fixed_t x, wl_fixed_t y) {
	struct swaylock_seat *seat = data;
	struct swaylock_state *state = seat->state;
	swaylock_handle_touch(state, TOUCH_EVENT_MOTION, wl_fixed_to_int(x),
			      wl_fixed_to_int(y));
}

static void wl_touch_frame(void *data, struct wl_touch *touch) {
	//Unneded, left blank
}

static void wl_touch_orientation(void *data, struct wl_touch *touch, int32_t id,
				 wl_fixed_t orientation) {
	//"Who cares" is so cruel
}

static void wl_touch_cancel(void *data, struct wl_touch *touch) {
	//Unneeded, left blank
}

static void wl_touch_shape(void *data, struct wl_touch *touch, int32_t id,
			   wl_fixed_t major, wl_fixed_t minor) {
	//Unneeded, left blank
}

static const struct wl_touch_listener touch_listener = {
	.down = wl_touch_down,
	.up = wl_touch_up,
	.frame = wl_touch_frame,
	.motion = wl_touch_motion,
	.orientation = wl_touch_orientation,
	.shape = wl_touch_shape,
	.cancel = wl_touch_cancel,
};

static void seat_handle_capabilities(void *data, struct wl_seat *wl_seat,
				     enum wl_seat_capability caps) {
	struct swaylock_seat *seat = data;
	if (seat->pointer) {
		wl_pointer_release(seat->pointer);
		seat->pointer = NULL;
	}
	if (seat->keyboard) {
		wl_keyboard_release(seat->keyboard);
		seat->keyboard = NULL;
	}

	if (seat->touch) {
		wl_touch_release(seat->touch);
		seat->touch = NULL;
	}

	if ((caps & WL_SEAT_CAPABILITY_POINTER)) {
		seat->pointer = wl_seat_get_pointer(wl_seat);
		wl_pointer_add_listener(seat->pointer, &pointer_listener, NULL);
	}
	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
		seat->keyboard = wl_seat_get_keyboard(wl_seat);
		wl_keyboard_add_listener(seat->keyboard, &keyboard_listener,
					 seat->state);
	}

	if (caps & WL_SEAT_CAPABILITY_TOUCH) {
		seat->touch = wl_seat_get_touch(wl_seat);
		seat->state->touch.pressed = false;
		wl_touch_add_listener(seat->touch, &touch_listener, seat);
	}
}

static void seat_handle_name(void *data, struct wl_seat *wl_seat,
			     const char *name) {
	// Who cares
}

const struct wl_seat_listener seat_listener = {
	.capabilities = seat_handle_capabilities,
	.name = seat_handle_name,
};
