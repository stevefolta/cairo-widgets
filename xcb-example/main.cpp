#include "ExampleWindow.h"
#include "XCBConnection.h"
#include "XKBKeyboard.h"
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <cairo/cairo-xcb.h>
#include <poll.h>
#include <map>
#include <iostream>

// For X11/keysymdef.h, we need to specify which types of keys we want.
#define XK_MISCELLANY
#include <X11/keysymdef.h>

XKBKeyboard keyboard;
std::map<xcb_window_t, XCBWindow*> windows_by_id;
bool running = true;
struct Atom {
	xcb_atom_t atom = 0;
	xcb_intern_atom_cookie_t cookie = {};
	};
std::map<std::string, Atom> atoms = {
	{ "WM_PROTOCOLS", {} }, { "WM_DELETE_WINDOW", {} }, { "UTF8_STRING", {} }, { "WM_NAME", {} },
	};

extern void handle_x11_event(xcb_generic_event_t* event);
extern void handle_key_event(xcb_key_press_event_t* event);


int fail(const char* message)
{
	std::cerr << message << std::endl;
	return 1;
}

int main(int argc, const char* argv[])
{
	// Create the XCB connection.
	if (!xcb_connection.connect())
		fail("Can't open display.");
	// Set up XKB.
	if (!keyboard.open(xcb_connection.connection))
		return fail("Couldn't set up XKB.");

	// Create the window.
	ExampleWindow* example_window = new ExampleWindow();
	if (!example_window->is_valid())
		return fail("Couldn't open window");
	example_window->set_title("CairoWidgets Example");
	windows_by_id[example_window->x_window] = example_window;

	// Event loop.
	std::vector<struct pollfd> poll_fds = {
		{ xcb_get_file_descriptor(xcb_connection.connection), POLLIN, 0 },
		};
	while (running) {
		// Wait for X event, or timeout.
		int timeout_ms = -1;
		for (auto& it: windows_by_id) {
			int window_timeout = it.second->next_update_ms();
			if (window_timeout >= 0 && (timeout_ms < 0 || window_timeout < timeout_ms))
				timeout_ms = window_timeout;
			}
		int num_waiting = poll(poll_fds.data(), poll_fds.size(), timeout_ms);
		for (auto& it: windows_by_id) {
			auto window = it.second;
			if (window->next_update_ms() == 0)
				window->redraw();
			}
		if (num_waiting <= 0)
			continue;

		auto event = xcb_poll_for_event(xcb_connection.connection);
		if (event == nullptr)
			continue;

		while (true) {
			auto next_event = xcb_poll_for_event(xcb_connection.connection);
			if (next_event == nullptr)
				break;

			auto event_type = event->response_type & ~0x80;
			auto next_event_type = next_event->response_type & ~0x80;
			if (event_type == XCB_MOTION_NOTIFY && next_event_type == XCB_MOTION_NOTIFY) {
				// Merge adjacent mouse motion events.
				}
			else {
				handle_x11_event(event);
				xcb_connection.flush();
				}

			free(event);
			event = next_event;
			}

		if (event) {
			handle_x11_event(event);
			xcb_connection.flush();
			free(event);
			}
		}

	// Clean up.
	keyboard.close();
	xcb_connection.disconnect();

	return 0;
}


static XCBWindow* window_for(xcb_window_t x_window)
{
	auto it = windows_by_id.find(x_window);
	if (it == windows_by_id.end())
		return nullptr;
	return it->second;
}

void handle_x11_event(xcb_generic_event_t* event)
{
	auto event_type = event->response_type & ~0x80;
	switch (event_type) {
		case XCB_EXPOSE:
			{
			auto it = windows_by_id.find(((xcb_expose_event_t*) event)->window);
			if (it != windows_by_id.end())
				it->second->redraw();
			}
			break;

		case XCB_BUTTON_PRESS:
			{
			auto button_event = (xcb_button_press_event_t*) event;
			auto window = window_for(button_event->event);
			if (window) {
				window->mouse_pressed(button_event->event_x, button_event->event_y, button_event->detail);
				window->redraw();
				}
			}
			break;
		case XCB_BUTTON_RELEASE:
			{
			auto button_event = (xcb_button_release_event_t*) event;
			auto window = window_for(button_event->event);
			if (window) {
				window->mouse_released(button_event->event_x, button_event->event_y, button_event->detail);
				window->redraw();
				}
			}
		case XCB_MOTION_NOTIFY:
			{
			auto motion_event = (xcb_motion_notify_event_t*) event;
			auto window = window_for(motion_event->event);
			if (window) {
				window->mouse_moved(motion_event->event_x, motion_event->event_y);
				window->redraw();
				}
			}
			break;
		case XCB_KEY_PRESS:
			{
			auto key_event = (xcb_key_press_event_t*) event;
			handle_key_event(key_event);
			}
			break;
		case XCB_CONFIGURE_NOTIFY:
			{
			auto configure_event = (xcb_configure_notify_event_t*) event;
			auto window = window_for(configure_event->window);
			if (window) {
				window->resize(configure_event->width, configure_event->height);
				window->redraw();
				}
			}
			break;
		case XCB_CLIENT_MESSAGE:
			{
			auto client_message_event = (xcb_client_message_event_t*) event;
			if (client_message_event->data.data32[0] == xcb_connection.atom("WM_DELETE_WINDOW")) {
				auto window = window_for(client_message_event->window);
				if (window) {
					windows_by_id.erase(window->x_window);
					delete window;
					}
				if (windows_by_id.empty())
					running = false;
				}
			}
			break;
		default:
			if (event_type == keyboard.xkb_response_type)
				keyboard.handle_event(event);
			break;
		}
}


static const std::map<xcb_keysym_t, SpecialKey> special_keys = {
	{ XK_Left, LeftArrow }, { XK_Right, RightArrow }, { XK_Up, UpArrow }, { XK_Down, DownArrow },
	{ XK_Page_Up, PageUp }, { XK_Page_Down, PageDown }, { XK_Home, HomeKey }, { XK_End, EndKey },
	};

void handle_key_event(xcb_key_press_event_t* event)
{
	auto window = window_for(event->event);
	if (window == nullptr)
		return;

	auto keycode = event->detail;
	auto keysym = keyboard.keysym_for(keycode);
	auto it = special_keys.find(keysym);
	if (it != special_keys.end())
		window->special_key_pressed(it->second);
	else {
		auto codepoint = keyboard.unicode_for(keycode);
		if (codepoint)
			window->key_pressed(codepoint);
		}
	window->redraw();
}



