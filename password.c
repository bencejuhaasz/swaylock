#include <assert.h>
#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>
#include "comm.h"
#include "log.h"
#include "loop.h"
#include "seat.h"
#include "swaylock.h"
#include "buttons.h"
#include "unicode.h"

void clear_buffer(char *buf, size_t size) {
	// Use volatile keyword so so compiler can't optimize this out.
	volatile char *buffer = buf;
	volatile char zero = '\0';
	for (size_t i = 0; i < size; ++i) {
		buffer[i] = zero;
	}
}

void clear_password_buffer(struct swaylock_password *pw) {
	clear_buffer(pw->buffer, sizeof(pw->buffer));
	pw->len = 0;
}

static bool backspace(struct swaylock_password *pw) {
	if (pw->len != 0) {
		pw->buffer[--pw->len] = 0;
		return true;
	}
	return false;
}

static void append_ch(struct swaylock_password *pw, uint32_t codepoint) {
	size_t utf8_size = utf8_chsize(codepoint);
	if (pw->len + utf8_size + 1 >= sizeof(pw->buffer)) {
		// TODO: Display error
		return;
	}
	utf8_encode(&pw->buffer[pw->len], codepoint);
	pw->buffer[pw->len + utf8_size] = 0;
	pw->len += utf8_size;
}

static void clear_indicator(void *data) {
	struct swaylock_state *state = data;
	state->clear_indicator_timer = NULL;
	state->auth_state = AUTH_STATE_IDLE;
	damage_state(state);
}

void schedule_indicator_clear(struct swaylock_state *state) {
	if (state->clear_indicator_timer) {
		loop_remove_timer(state->eventloop, state->clear_indicator_timer);
	}
	state->clear_indicator_timer = loop_add_timer(
			state->eventloop, 3000, clear_indicator, state);
}

static void clear_password(void *data) {
	struct swaylock_state *state = data;
	state->clear_password_timer = NULL;
	state->auth_state = AUTH_STATE_CLEAR;
	clear_password_buffer(&state->password);
	damage_state(state);
	schedule_indicator_clear(state);
}

static void schedule_password_clear(struct swaylock_state *state) {
	if (state->clear_password_timer) {
		loop_remove_timer(state->eventloop, state->clear_password_timer);
	}
	state->clear_password_timer = loop_add_timer(
			state->eventloop, 10000, clear_password, state);
}

static void submit_password(struct swaylock_state *state) {
	if (state->args.ignore_empty && state->password.len == 0) {
		return;
	}

	state->auth_state = AUTH_STATE_VALIDATING;

	if (!write_comm_request(&state->password)) {
		state->auth_state = AUTH_STATE_INVALID;
		schedule_indicator_clear(state);
	}

	damage_state(state);
}

void swaylock_handle_key(struct swaylock_state *state,
		xkb_keysym_t keysym, uint32_t codepoint) {
	// Ignore input events if validating
	if (state->auth_state == AUTH_STATE_VALIDATING) {
		return;
	}

	switch (keysym) {
	case XKB_KEY_KP_Enter: /* fallthrough */
	case XKB_KEY_Return:
		submit_password(state);
		break;
	case XKB_KEY_Delete:
	case XKB_KEY_BackSpace:
		if (backspace(&state->password)) {
			state->auth_state = AUTH_STATE_BACKSPACE;
		} else {
			state->auth_state = AUTH_STATE_CLEAR;
		}
		damage_state(state);
		schedule_indicator_clear(state);
		schedule_password_clear(state);
		break;
	case XKB_KEY_Escape:
		clear_password_buffer(&state->password);
		state->auth_state = AUTH_STATE_CLEAR;
		damage_state(state);
		schedule_indicator_clear(state);
		break;
	case XKB_KEY_Caps_Lock:
	case XKB_KEY_Shift_L:
	case XKB_KEY_Shift_R:
	case XKB_KEY_Control_L:
	case XKB_KEY_Control_R:
	case XKB_KEY_Meta_L:
	case XKB_KEY_Meta_R:
	case XKB_KEY_Alt_L:
	case XKB_KEY_Alt_R:
	case XKB_KEY_Super_L:
	case XKB_KEY_Super_R:
		state->auth_state = AUTH_STATE_INPUT_NOP;
		damage_state(state);
		schedule_indicator_clear(state);
		schedule_password_clear(state);
		break;
	case XKB_KEY_m: /* fallthrough */
	case XKB_KEY_d:
	case XKB_KEY_j:
		if (state->xkb.control) {
			submit_password(state);
			break;
		}
		// fallthrough
	case XKB_KEY_c: /* fallthrough */
	case XKB_KEY_u:
		if (state->xkb.control) {
			clear_password_buffer(&state->password);
			state->auth_state = AUTH_STATE_CLEAR;
			damage_state(state);
			schedule_indicator_clear(state);
			break;
		}
		// fallthrough
	default:
		if (codepoint) {
			append_ch(&state->password, codepoint);
			state->auth_state = AUTH_STATE_INPUT;
			damage_state(state);
			schedule_indicator_clear(state);
			schedule_password_clear(state);
		}
		break;
	}
}



static void continuous_render(void *data) {
  struct swaylock_state *state = data;
  if (state->render_state == RENDER_STATE_FIRST_UNLOCK
      || state->render_state == RENDER_STATE_FIRST_UNLOCK_DONE) {
    damage_state(state);
    state->continuous_render_timer =
      loop_add_timer(state->eventloop, 16, continuous_render, state);
  }
  
}

void schedule_continuous_render(struct swaylock_state *state) {
	if (state->continuous_render_timer) {
		loop_remove_timer(state->eventloop,
				  state->continuous_render_timer);
	}
	state->continuous_render_timer = loop_add_timer(
		state->eventloop, 16, continuous_render, state);
}


static void first_unlock_done_complete(void *data) {
  struct swaylock_state *state = data;
  state->render_state = RENDER_STATE_PIN;
  damage_state(state);
  printf("unlock done\n");
}

void schedule_first_unlock_done_complete(struct swaylock_state *state) {
  if (state->first_unlock_done_timer) {
    loop_remove_timer(state->eventloop, state->first_unlock_done_timer);
  }
  state->first_unlock_done_timer = loop_add_timer(state->eventloop, 1000, first_unlock_done_complete, state);
}

static void first_unlock(void *data) {
	struct swaylock_state *state = data;
	if (state->render_state == RENDER_STATE_FIRST_UNLOCK) {
		state->render_state = RENDER_STATE_FIRST_UNLOCK_DONE;
		schedule_first_unlock_done_complete(state);
		damage_state(state);
	}
}

void schedule_first_unlock_complete(struct swaylock_state *state) {
	if (state->first_unlock_timer) {
		loop_remove_timer(state->eventloop, state->first_unlock_timer);
	}
	state->first_unlock_timer =
		loop_add_timer(state->eventloop, 5000, first_unlock, state);
}

void swaylock_handle_touch(struct swaylock_state *state,
			   enum touch_event event, int x, int y) {
	switch (event) {
	case TOUCH_EVENT_DOWN:
		if (!state->touch.pressed) {
			state->touch.pressed = true;
			state->touch.x = x;
			state->touch.y = y;
			if (state->render_state == RENDER_STATE_INITIAL) {
			  /*	state->render_state = RENDER_STATE_FIRST_UNLOCK;
				schedule_first_unlock_complete(state);
				schedule_continuous_render(state);*/
			  state->render_state = RENDER_STATE_PIN;
			}
		}
		break;
	case TOUCH_EVENT_UP:
		state->touch.pressed = false;
		if (state->render_state == RENDER_STATE_FIRST_UNLOCK) {
			state->render_state = RENDER_STATE_INITIAL;
			loop_remove_timer(state->eventloop,
					  state->first_unlock_timer);
		}
		break;
	case TOUCH_EVENT_MOTION:
		if (state->render_state == RENDER_STATE_FIRST_UNLOCK) {
			if (x > 100 &&
			    y > 100) { //TODO check if it's just my touch
				state->touch.x = x;
				state->touch.y = y;
			}
		}
		break;
	}
	damage_state(state);
}

void swaylock_touch_recalculate_keys(struct swaylock_state *state, uint32_t new_width, uint32_t new_height) {

  state->to
  
  int button_spacing = 50;
  int button_width = (buffer_width - button_spacing * 4) / 3;
  int button_height = (buffer_width - button_spacing * 5) / 4;
  
}
