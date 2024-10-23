#pragma once

#include "WaylandXKBKeyboard.h"
#include "TimeSeconds.h"
#include <wayland-client.h>
#include <stdint.h>
#include <list>
#include <map>

class WaylandCairoWindow;


class WaylandDisplay {
	public:
		struct wl_display* display = nullptr;
		struct wl_registry* registry = nullptr;
		struct wl_compositor* compositor = nullptr;
		struct wl_shm* shm = nullptr;
		struct xdg_wm_base* xdg_wm_base = nullptr;
		struct wl_seat* seat = nullptr;
		struct zxdg_decoration_manager_v1* xdg_decoration_manager_v1 = nullptr;
		struct wp_cursor_shape_manager_v1* cursor_shape_manager_v1 = nullptr;

		bool connect();
		void disconnect();

		void flush() {
			wl_display_flush(display);
			}

		void add_window(WaylandCairoWindow* window);
		void remove_window(WaylandCairoWindow* window);
		bool has_windows() { return !windows.empty(); }
		int next_update_ms();
		bool redraw_pending_windows(); 	// true => successful.

		// Cursor types, in addition to those in Widget.
		enum {
			WindowMoveCursor = 256,
			ResizeTopLeftCursor, ResizeTopCursor, ResizeTopRightCursor,
			ResizeLeftCursor, ResizeRightCursor,
			ResizeBottomLeftCursor, ResizeBottomCursor, ResizeBottomRightCursor,
			};

		void use_cursor(int cursor_type);

	protected:
		std::list<WaylandCairoWindow*> windows;
		WaylandCairoWindow* pointer_window = nullptr;
		WaylandCairoWindow* key_window = nullptr;
		struct wl_pointer* pointer = nullptr;
		struct wl_keyboard* keyboard = nullptr;
		double last_pointer_x, last_pointer_y;
		WaylandXKBKeyboard xkb_keyboard;
		struct wl_cursor_theme* cursor_theme = nullptr;
		struct wl_surface* cursor_surface = nullptr;
		std::map<int, struct wl_cursor*> cursors;
		struct wp_cursor_shape_device_v1* cursor_shape_device = nullptr;
		uint32_t pointer_enter_serial = 0;
		int32_t key_repeat_delay_ms = 0, key_repeat_rate_cps = 0;
		TimeSeconds next_key_repeat = { 0, 0 };
		uint32_t repeat_unicode = 0;
		int repeat_special_key = -1;

		WaylandCairoWindow* window_for(struct wl_surface* wayland_surface);
		void seat_capabilities(uint32_t capabilities);
		void seat_name(const char* name);
		void pointer_enter(uint32_t serial, struct wl_surface* surface, wl_fixed_t x, wl_fixed_t y);
		void pointer_leave(uint32_t serial, struct wl_surface* surface);
		void pointer_motion(uint32_t time, wl_fixed_t x, wl_fixed_t y);
		void pointer_button(uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
		void pointer_axis(uint32_t time, uint32_t axis, wl_fixed_t value);
		void keyboard_keymap(uint32_t format, int fd, uint32_t size);
		void keyboard_enter(uint32_t serial, struct wl_surface* surface, struct wl_array* keys);
		void keyboard_leave(uint32_t serial, struct wl_surface* surface);
		void keyboard_key(uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
		void keyboard_modifiers(
			uint32_t serial,
			uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
		void keyboard_repeat_info(int32_t rate, int32_t delay);

		void setup_cursors();
		void cleanup_cursors();
	};

extern WaylandDisplay wayland_display;

