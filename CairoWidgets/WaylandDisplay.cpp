#include "WaylandDisplay.h"
#include "WaylandCairoWindow.h"
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "cursor-shape-v1-client-protocol.h"
#include <vector>
#include <iostream>
#include <linux/input-event-codes.h>
#include <wayland-cursor.h>
#include <string.h>
#include <unistd.h>


WaylandDisplay wayland_display;


bool WaylandDisplay::connect()
{
	display = wl_display_connect(nullptr);
	if (display == nullptr)
		return false;

	registry = wl_display_get_registry(display);
	static const struct xdg_wm_base_listener xdg_wm_base_listener = {
		.ping = [](void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial) {
			xdg_wm_base_pong(xdg_wm_base, serial);
			},
		};
	static const struct wl_seat_listener wl_seat_listener = {
		.capabilities = [](void* data, struct wl_seat* seat, uint32_t capabilities) {
			((WaylandDisplay*) data)->seat_capabilities(capabilities);
			},
		.name = [](void* data, struct wl_seat* seat, const char* name) {
			((WaylandDisplay*) data)->seat_name(name);
			}
		};
	static const struct wl_registry_listener registry_listener = {
		.global = [](void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
			auto self = (WaylandDisplay*) data;
			if (strcmp(interface, wl_compositor_interface.name) == 0)
				self->compositor = (wl_compositor*) wl_registry_bind(registry, name, &wl_compositor_interface, 4);
			else if (strcmp(interface, wl_shm_interface.name) == 0)
				self->shm = (wl_shm*) wl_registry_bind(registry, name, &wl_shm_interface, 1);
			else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
				self->xdg_wm_base = (struct xdg_wm_base*) wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
				xdg_wm_base_add_listener(self->xdg_wm_base, &xdg_wm_base_listener, self);
				}
			else if (strcmp(interface, wl_seat_interface.name) == 0) {
				self->seat = (struct wl_seat*) wl_registry_bind(registry, name, &wl_seat_interface, 7);
				wl_seat_add_listener(self->seat, &wl_seat_listener, self);
				}
			else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
				self->xdg_decoration_manager_v1 =
					(struct zxdg_decoration_manager_v1*)
					wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1);
				}
			else if (strcmp(interface, wp_cursor_shape_manager_v1_interface.name) == 0) {
				self->cursor_shape_manager_v1 =
					(struct wp_cursor_shape_manager_v1*)
					wl_registry_bind(registry, name, &wp_cursor_shape_manager_v1_interface, 1);
				}
			},
		.global_remove = [](void* data, struct wl_registry* registry, uint32_t name) {},
		};
	wl_registry_add_listener(registry, &registry_listener, this);
	wl_display_roundtrip(display);

	if (!xkb_keyboard.open()) {
		std::cerr << "Couldn't set up XKB." << std::endl;
		return false;
		}

	setup_cursors();

	return true;
}


void WaylandDisplay::disconnect()
{
	cleanup_cursors();

	// Destroy all windows.
	while (!windows.empty())
		delete(windows.front());

	if (xdg_decoration_manager_v1) {
		zxdg_decoration_manager_v1_destroy(xdg_decoration_manager_v1);
		xdg_decoration_manager_v1 = nullptr;
		}
	if (cursor_shape_manager_v1) {
		wp_cursor_shape_manager_v1_destroy(cursor_shape_manager_v1);
		cursor_shape_manager_v1 = nullptr;
		}
	if (pointer) {
		wl_pointer_release(pointer);
		pointer = nullptr;
		}
	if (keyboard) {
		wl_keyboard_release(keyboard);
		keyboard = nullptr;
		}
	if (shm) {
		wl_shm_destroy(shm);
		shm = nullptr;
		}
	if (display) {
		wl_compositor_destroy(compositor);
		wl_registry_destroy(registry);
		wl_display_disconnect(display);
		display = nullptr;
		registry = nullptr;
		compositor = nullptr;
		}
	xkb_keyboard.close();
}


void WaylandDisplay::add_window(WaylandCairoWindow* window)
{
	windows.push_back(window);
}

void WaylandDisplay::remove_window(WaylandCairoWindow* window)
{
	windows.remove(window);
	if (window == pointer_window)
		pointer_window = nullptr;
	if (window == key_window)
		key_window = nullptr;
}

int WaylandDisplay::next_update_ms()
{
	int timeout_ms = -1;
	for (auto window: windows) {
		int window_timeout = window->next_update_ms();
		if (window_timeout >= 0 && (timeout_ms < 0 || window_timeout < timeout_ms))
			timeout_ms = window_timeout;
		}

	// Key repeat.
	if (key_window && next_key_repeat.is_valid()) {
		int next_repeat_ms = next_key_repeat.ms_left();
		if (timeout_ms < 0 || next_repeat_ms < timeout_ms)
			timeout_ms = (next_repeat_ms > 0 ? next_repeat_ms : 0);
		}

	return timeout_ms;
}

void WaylandDisplay::redraw_pending_windows()
{
	for (auto window: windows) {
		if (window->redraw_requested || window->next_update_ms() == 0) {
			window->redraw();
			window->redraw_requested = false;
			}
		}

	if (key_window && next_key_repeat.seconds() > 0 && TimeSeconds::now() >= next_key_repeat) {
		if (repeat_unicode > 0)
			key_window->key_pressed(repeat_unicode);
		else if (repeat_special_key >= 0)
			key_window->special_key_pressed((SpecialKey) repeat_special_key);
		key_window->redraw();

		int key_repeat_rate_ms = 1000 / key_repeat_rate_cps;
		next_key_repeat = { key_repeat_rate_ms / 1000, (key_repeat_rate_ms % 1000) * 1000000 };
		next_key_repeat = TimeSeconds::now() + next_key_repeat;
		}
}

WaylandCairoWindow* WaylandDisplay::window_for(struct wl_surface* wayland_surface)
{
	for (auto window: windows) {
		if (window->wayland_surface == wayland_surface)
			return window;
		}
	return nullptr;
}


void WaylandDisplay::seat_capabilities(uint32_t capabilities)
{
	if ((capabilities & WL_SEAT_CAPABILITY_POINTER) != 0 && pointer == nullptr) {
		pointer = wl_seat_get_pointer(seat);
		wl_pointer_set_user_data(pointer, this);
		static const struct wl_pointer_listener pointer_listener = {
			.enter = [](void* data, struct wl_pointer* pointer, uint32_t serial, struct wl_surface* surface, wl_fixed_t x, wl_fixed_t y) {
				((WaylandDisplay*) data)->pointer_enter(serial, surface, x, y); },
			.leave = [](void* data, struct wl_pointer* pointer, uint32_t serial, struct wl_surface* surface) {
				((WaylandDisplay*) data)->pointer_leave(serial, surface);
				},
			.motion = [](void* data, struct wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
				((WaylandDisplay*) data)->pointer_motion(time, x, y);
				},
			.button = [](void* data, struct wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
				((WaylandDisplay*) data)->pointer_button(serial, time, button, state);
				},
			.axis = [](void* data, struct wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {
				((WaylandDisplay*) data)->pointer_axis(time, axis, value);
				},
			.frame = [](void* data, struct wl_pointer* pointer) {},
			.axis_source = [](void* data, struct wl_pointer* pointer, uint32_t source) {},
			.axis_stop = [](void* data, struct wl_pointer* pointer, uint32_t time, uint32_t axis) {},
			.axis_discrete = [](void* data, struct wl_pointer* pointer, uint32_t axis, int32_t discrete) {},
			};
		wl_pointer_add_listener(pointer, &pointer_listener, this);
		cleanup_cursors();
		setup_cursors();
		}
	else if ((capabilities & WL_SEAT_CAPABILITY_POINTER) == 0 && pointer) {
		wl_pointer_release(pointer);
		pointer = nullptr;
		cleanup_cursors();
		setup_cursors();
		}

	if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0 && keyboard == nullptr) {
		keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_set_user_data(keyboard, this);
		static const struct wl_keyboard_listener keyboard_listener = {
			.keymap = [](void* data, struct wl_keyboard* keyboard, uint32_t format, int fd, uint32_t size) {
				((WaylandDisplay*) data)->keyboard_keymap(format, fd, size);
				},
			.enter = [](void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface, struct wl_array* keys) {
				((WaylandDisplay*) data)->keyboard_enter(serial, surface, keys);
				},
			.leave = [](void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface) {
				((WaylandDisplay*) data)->keyboard_leave(serial, surface);
				},
			.key = [](void* data, struct wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
				((WaylandDisplay*) data)->keyboard_key(serial, time, key, state);
				},
			.modifiers = [](
				void* data, struct wl_keyboard* keyboard, uint32_t serial,
				uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
				((WaylandDisplay*) data)->keyboard_modifiers(serial, mods_depressed, mods_latched, mods_locked, group);
				},
			.repeat_info = [](void* data, struct wl_keyboard* keyboard, int32_t rate, int32_t delay) {
				((WaylandDisplay*) data)->keyboard_repeat_info(rate, delay);
				}
			};
		wl_keyboard_add_listener(keyboard, &keyboard_listener, this);
		}
	else if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) == 0 && keyboard) {
		wl_keyboard_release(keyboard);
		keyboard = nullptr;
		}
}

void WaylandDisplay::seat_name(const char* name)
{
	// Don't really care about this.
}

void WaylandDisplay::pointer_enter(uint32_t serial, struct wl_surface* surface, wl_fixed_t x, wl_fixed_t y)
{
	pointer_window = window_for(surface);
	if (pointer_window) {
		last_pointer_x = wl_fixed_to_double(x);
		last_pointer_y = wl_fixed_to_double(y);
		pointer_window->entered(last_pointer_x, last_pointer_y);
		}
	pointer_enter_serial = serial;
}

void WaylandDisplay::pointer_leave(uint32_t serial, struct wl_surface* surface)
{
	pointer_window = nullptr;
}

void WaylandDisplay::pointer_motion(uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	if (pointer_window) {
		last_pointer_x = wl_fixed_to_double(x);
		last_pointer_y = wl_fixed_to_double(y);
		pointer_window->mouse_moved(last_pointer_x, last_pointer_y);
		}
}

void WaylandDisplay::pointer_button(uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	if (pointer_window == nullptr)
		return;

	int adjusted_button = button - BTN_MOUSE + 1;
	if (state == WL_POINTER_BUTTON_STATE_PRESSED)
		pointer_window->mouse_pressed(last_pointer_x, last_pointer_y, adjusted_button, serial);
	else if (state == WL_POINTER_BUTTON_STATE_RELEASED)
		pointer_window->mouse_released(last_pointer_x, last_pointer_y, adjusted_button);
}

void WaylandDisplay::pointer_axis(uint32_t time, uint32_t axis, wl_fixed_t value)
{
	if (pointer_window == nullptr)
		return;

	if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
		if (value > 0)
			pointer_window->scroll_down(last_pointer_x, last_pointer_y);
		else
			pointer_window->scroll_up(last_pointer_x, last_pointer_y);
		}
}


void WaylandDisplay::keyboard_keymap(uint32_t format, int fd, uint32_t size)
{
	if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
		close(fd);
		return;
		}

	xkb_keyboard.use_keymap_fd(fd, size);
}

void WaylandDisplay::keyboard_enter(uint32_t serial, struct wl_surface* surface, struct wl_array* keys)
{
	key_window = window_for(surface);
}

void WaylandDisplay::keyboard_leave(uint32_t serial, struct wl_surface* surface)
{
	key_window = nullptr;
}

#define XK_MISCELLANY
#include <X11/keysymdef.h>
static const std::map<uint32_t, SpecialKey> special_keys = {
	{ XK_Left, LeftArrow }, { XK_Right, RightArrow }, { XK_Up, UpArrow }, { XK_Down, DownArrow },
	{ XK_Page_Up, PageUp }, { XK_Page_Down, PageDown }, { XK_Home, HomeKey }, { XK_End, EndKey },
	};

void WaylandDisplay::keyboard_key(uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	next_key_repeat.clear();
	repeat_unicode = 0;
	repeat_special_key = -1;

	if (key_window == nullptr)
		return;
	if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
		return;

	auto xkb_key = key + 8;
	auto keysym = xkb_keyboard.keysym_for(xkb_key);
	auto it = special_keys.find(keysym);
	if (it != special_keys.end()) {
		key_window->special_key_pressed(it->second);
		repeat_special_key = it->second;
		}
	else {
		auto codepoint = xkb_keyboard.unicode_for(xkb_key);
		if (codepoint)
			key_window->key_pressed(codepoint);
		repeat_unicode = codepoint;
		}
	key_window->redraw();

	// Set up to repeat it.
	if (key_repeat_delay_ms > 0) {
		next_key_repeat = { key_repeat_delay_ms / 1000, (key_repeat_delay_ms % 1000) * 1000000 };
		next_key_repeat = TimeSeconds::now() + next_key_repeat;
		}
}

void WaylandDisplay::keyboard_modifiers(
	uint32_t serial,
	uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	xkb_keyboard.update_modifiers(mods_depressed, mods_latched, mods_locked, group);
}

void WaylandDisplay::keyboard_repeat_info(int32_t rate, int32_t delay)
{
	key_repeat_delay_ms = delay;
	key_repeat_rate_cps = rate;
}


struct CursorSpec {
	int cursor;
	std::vector<std::string> names;
	};
// These come from weston/clients/window.c, which says it got them from
// <https://bugs.kde.org/attachment.cgi?id=67313>.
static const std::vector<CursorSpec> cursor_specs = {
	{ Widget::PointerCursor, { "left_ptr", "default", "top_left_arrow", "left-arrow" } },
	{ Widget::TextCursor, { "xterm", "ibeam", "text" } },
	{ WaylandDisplay::WindowMoveCursor, { "grabbing", "closedhand" } },
	{ WaylandDisplay::ResizeTopLeftCursor, { "top_left_corner", "nw-resize", "size_fdiag" } },
	{ WaylandDisplay::ResizeTopCursor, { "top_side", "n-resize", "size_ver" } },
	{ WaylandDisplay::ResizeTopRightCursor, { "top_right_corner", "nw-resize", "size_bdiag" } },
	{ WaylandDisplay::ResizeLeftCursor, { "left_side", "w-resize", "size_hor" } },
	{ WaylandDisplay::ResizeRightCursor, { "right_side", "e-resize", "size_hor" } },
	{ WaylandDisplay::ResizeBottomLeftCursor, { "bottom_left_corner", "sw-resize", "size_bdiag" } },
	{ WaylandDisplay::ResizeBottomCursor, { "bottom_side", "s-resize", "size_ver" } },
	{ WaylandDisplay::ResizeBottomRightCursor, { "bottom_right_corner", "se-resize", "side_fdiag" } },
	};


void WaylandDisplay::setup_cursors()
{
	if (cursor_shape_manager_v1 && pointer) {
		cursor_shape_device = wp_cursor_shape_manager_v1_get_pointer(cursor_shape_manager_v1, pointer);
		if (cursor_shape_device)
			return;
		}

	if (shm == nullptr || compositor == nullptr)
		return;

	cursor_surface = wl_compositor_create_surface(compositor);
	if (cursor_surface == nullptr)
		return;

	cursor_theme = wl_cursor_theme_load(nullptr, 24, shm);
	if (cursor_theme == nullptr)
		return;

	for (const auto& spec: cursor_specs) {
		wl_cursor* cursor = nullptr;
		for (const auto& name: spec.names) {
			cursor = wl_cursor_theme_get_cursor(cursor_theme, name.c_str());
			if (cursor)
				break;
			}
		if (cursor)
			cursors[spec.cursor] = cursor;
		}
}

void WaylandDisplay::cleanup_cursors()
{
	if (cursor_shape_device) {
		wp_cursor_shape_device_v1_destroy(cursor_shape_device);
		cursor_shape_device = nullptr;
		}

	// It's not documented (of course), but apparently the cursor theme owns all
	// the cursors and their buffers.
	cursors.clear();
	if (cursor_theme) {
		wl_cursor_theme_destroy(cursor_theme);
		cursor_theme = nullptr;
		}
	if (cursor_surface) {
		wl_surface_destroy(cursor_surface);
		cursor_surface = nullptr;
		}
}


void WaylandDisplay::use_cursor(int cursor_type)
{
	// Use server-side cursors if available.
	if (cursor_shape_device) {
		static const std::map<int, uint32_t> shapes = {
			{ Widget::PointerCursor, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT },
			{ Widget::TextCursor, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT },
			{ WaylandDisplay::WindowMoveCursor, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_GRABBING },
			{ WaylandDisplay::ResizeTopLeftCursor, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NE_RESIZE },
			{ WaylandDisplay::ResizeTopCursor, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_N_RESIZE },
			{ WaylandDisplay::ResizeTopRightCursor, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NE_RESIZE },
			{ WaylandDisplay::ResizeLeftCursor, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_W_RESIZE },
			{ WaylandDisplay::ResizeRightCursor, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_E_RESIZE },
			{ WaylandDisplay::ResizeBottomLeftCursor, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_SE_RESIZE },
			{ WaylandDisplay::ResizeBottomCursor, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_S_RESIZE },
			{ WaylandDisplay::ResizeBottomRightCursor, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_SW_RESIZE },
			};
		auto it = shapes.find(cursor_type);
		if (it != shapes.end()) {
			wp_cursor_shape_device_v1_set_shape(cursor_shape_device, pointer_enter_serial, it->second);
			return;
			}
		}

	// The rest is for client-side cursors.

	if (cursor_surface == nullptr)
		return;

	auto it = cursors.find(cursor_type);
	if (it == cursors.end())
		return;
	auto cursor = it->second;
	auto cursor_image = cursor->images[0];
	auto buffer = wl_cursor_image_get_buffer(cursor_image);
	if (buffer == nullptr)
		return;
	wl_surface_attach(cursor_surface, buffer, 0, 0);
	wl_surface_damage(cursor_surface, 0, 0, cursor_image->width, cursor_image->height);
	wl_surface_commit(cursor_surface);
	wl_pointer_set_cursor(
		pointer, pointer_enter_serial, cursor_surface,
		cursor_image->hotspot_x, cursor_image->hotspot_y);
}




