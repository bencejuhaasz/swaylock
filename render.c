#include <math.h>
#include <stdlib.h>
#include <wayland-client.h>
#include "cairo.h"
#include "background-image.h"
#include "swaylock.h"
#include "loop.h"
#include "buttons.h"

 const char *buttons[12] = {
      "1", "2", "3",
      "4", "5", "6",
      "7", "8", "9",
      "⌫", "0", "⏎"};

#define M_PI 3.14159265358979323846

static void set_color_for_state(cairo_t *cairo, struct swaylock_state *state,
				struct swaylock_colorset *colorset)
{
	if (state->auth_state == AUTH_STATE_VALIDATING) {
		cairo_set_source_u32(cairo, colorset->verifying);
	} else if (state->auth_state == AUTH_STATE_INVALID) {
		cairo_set_source_u32(cairo, colorset->wrong);
	} else {
		if (state->xkb.caps_lock &&
		    state->args.show_caps_lock_indicator) {
			cairo_set_source_u32(cairo, colorset->caps_lock);
		} else if (state->xkb.caps_lock &&
			   !state->args.show_caps_lock_indicator &&
			   state->args.show_caps_lock_text) {
			uint32_t inputtextcolor = state->args.colors.text.input;
			state->args.colors.text.input =
				state->args.colors.text.caps_lock;
			cairo_set_source_u32(cairo, colorset->input);
			state->args.colors.text.input = inputtextcolor;
		} else {
			cairo_set_source_u32(cairo, colorset->input);
		}
	}
}

void render_frame_background(struct swaylock_surface *surface)
{
	struct swaylock_state *state = surface->state;

	int buffer_width = surface->width * surface->scale;
	int buffer_height = surface->height * surface->scale;
	if (buffer_width == 0 || buffer_height == 0) {
		return; // not yet configured
	}

	wl_subsurface_set_position(surface->subsurface, 0, 0);
	surface->current_buffer = get_next_buffer(state->shm, surface->buffers,
						  buffer_width, buffer_height);
	if (surface->current_buffer == NULL) {
		return;
	}

	cairo_t *cairo = surface->current_buffer->cairo;
	cairo_set_antialias(cairo, CAIRO_ANTIALIAS_BEST);

	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_u32(cairo, state->args.colors.background);
	cairo_paint(cairo);
	if (surface->image && state->args.mode != BACKGROUND_MODE_SOLID_COLOR) {
		cairo_set_operator(cairo, CAIRO_OPERATOR_OVER);
		render_background_image(cairo, surface->image, state->args.mode,
					buffer_width, buffer_height);
	}
	cairo_restore(cairo);
	cairo_identity_matrix(cairo);

	wl_surface_set_buffer_scale(surface->surface, surface->scale);
	wl_surface_attach(surface->surface, surface->current_buffer->buffer, 0,
			  0);
	wl_surface_damage(surface->surface, 0, 0, surface->width,
			  surface->height);
	wl_surface_commit(surface->surface);
}

bool render_prepare_surface(struct swaylock_surface *surface, int subsurf_xpos,
			    int subsurf_ypos, int buffer_width,
			    int buffer_height) {
	struct swaylock_state *state = surface->state;
	wl_subsurface_set_position(surface->subsurface, subsurf_xpos,
				   subsurf_ypos);

	surface->current_buffer =
		get_next_buffer(state->shm, surface->indicator_buffers,
				buffer_width, buffer_height);
	if (surface->current_buffer == NULL) {
		return false;
	}

	// Hide subsurface until we want it visible
	wl_surface_attach(surface->child, NULL, 0, 0);
	wl_surface_commit(surface->child);

	cairo_t *cairo = surface->current_buffer->cairo;
	cairo_set_antialias(cairo, CAIRO_ANTIALIAS_BEST);
	cairo_identity_matrix(cairo);

	// Clear
	cairo_save(cairo);
	cairo_set_source_rgba(cairo, 0, 0, 0, 0);
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cairo);
	cairo_restore(cairo);

	return true;
}

void render_frame_touch_pin(struct swaylock_surface *surface) {
	struct swaylock_state *state = surface->state;
	uint32_t buffer_width = state->touch.buttons_area_width;
	uint32_t buffer_height = state->touch.buttons_area_height;
	uint32_t button_spacing = state->touch.button_spacing;
	uint32_t button_width = state->touch.button_width;
	uint32_t button_height = state->touch.button_height;
	uint32_t text_area_height = state->touch.text_area_height;

	if (!render_prepare_surface(surface,
				    (surface->width - buffer_width) / 2,
				    (surface->height - buffer_height) / 2,
				    buffer_width, buffer_height)) {
		return;
	}

	cairo_t *cairo = surface->current_buffer->cairo;

	cairo_set_line_width(cairo, state->args.thickness * surface->scale);
	cairo_set_line_join(cairo, CAIRO_LINE_JOIN_ROUND);

	cairo_select_font_face(cairo, state->args.font, CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_NORMAL);
	uint32_t font_size = (state->args.font_size > 0 && state->args.font_size < 100) ? state->args.font_size : 50;
	cairo_set_font_size(cairo, font_size);

	/*Show input text*/
	set_color_for_state(cairo, state, &state->args.colors.text);
	char *pwline;
	switch (state->auth_state) {
	case AUTH_STATE_VALIDATING:
		pwline = "validating";
		break;
	case AUTH_STATE_INVALID:
		pwline = "wrong";
		break;
	default:
		pwline = calloc(state->password.len, sizeof(char));
		for (int i = 0; i < (int)state->password.len; i++) {
			pwline[i] = '*';
		}
	}
	cairo_text_extents_t pw_extents;
	cairo_text_extents(cairo, pwline, &pw_extents);

	cairo_move_to(cairo, buffer_width / 2 - pw_extents.width / 2, text_area_height - font_size / 2);
	cairo_show_text(cairo, pwline);

	/*show separator line*/
	set_color_for_state(cairo, state, &state->args.colors.line);
	cairo_move_to(cairo, button_spacing, text_area_height);
	cairo_line_to(cairo, buffer_width - button_spacing, text_area_height);
	cairo_stroke(cairo);

	/*show pinpad*/	
	int32_t pressed_button = state->touch.current_pressed;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 3; j++) {
		  int current_button = i * 3 + j;

		  //button rectangle
		  uint32_t fill_color =
			  (pressed_button == current_button) ?
				  state->args.colors.button_background_pressed :
				  state->args.colors.button_background;
		  cairo_set_source_u32(cairo, fill_color);
		  cairo_rectangle(cairo,
				  button_spacing * (j + 1) + button_width * j,
				  button_spacing * (i + 1) + button_height * i + text_area_height,
				  button_width, button_height);
		  cairo_fill(cairo);

		  //button border
		  cairo_set_source_u32(cairo, state->args.colors.button_border);
		  cairo_rectangle(cairo,
				  button_spacing * (j + 1) + button_width * j,
				  button_spacing * (i + 1) + button_height * i + text_area_height,
				  button_width, button_height);
		  cairo_stroke(cairo);

		  //button text
		  cairo_set_source_u32(cairo, state->args.colors.button_text);
		  cairo_text_extents_t extents;
		  cairo_text_extents(cairo, buttons[i * 3 + j], &extents);
		  cairo_move_to(cairo,
				button_spacing * (j + 1) + button_width * j +
					button_width / 2 - extents.width / 2,
				button_spacing * (i + 1) + button_height * i +
					button_height / 2 + extents.height / 2 + text_area_height);
		  cairo_show_text(cairo, buttons[current_button]);
		}
	}
}


void render_frame(struct swaylock_surface *surface) {
	struct swaylock_state *state = surface->state;

	render_frame_background(surface);

	switch (state->render_state) {
	case RENDER_STATE_INITIAL:
		break;
	case RENDER_STATE_PIN:
		render_frame_touch_pin(surface);
		break;
	default:
		break;
	}

	wl_surface_set_buffer_scale(surface->child, surface->scale);
	wl_surface_attach(surface->child, surface->current_buffer->buffer, 0,
			  0);
	wl_surface_damage(surface->child, 0, 0, surface->current_buffer->width,
			  surface->current_buffer->height);
	wl_surface_commit(surface->child);

	wl_surface_commit(surface->surface);
	
}

void render_frames(struct swaylock_state *state) {
	struct swaylock_surface *surface;
	wl_list_for_each(surface, &state->surfaces, link)
	{
		render_frame(surface);
	}
}
