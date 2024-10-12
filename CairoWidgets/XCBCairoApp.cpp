#include "XCBCairoApp.h"
#include "XCBCairoWindow.h"
#include <cairo/cairo-xcb.h>
#include <poll.h>
#include <map>
#include <iostream>

// For X11/keysymdef.h, we need to specify which types of keys we want.
#define XK_MISCELLANY
#include <X11/keysymdef.h>


XCBCairoApp::XCBCairoApp()
{
	atoms = {
		{ "WM_PROTOCOLS", {} }, { "WM_DELETE_WINDOW", {} }, { "UTF8_STRING", {} }, { "WM_NAME", {} },
		};

	// Create the XCB connection.
	if (!xcb_connection.connect()) {
		std::cerr << "Can't open display." << std::endl;
		return;
		}
	// Set up XKB.
	if (!keyboard.open(xcb_connection.connection)) {
		std::cerr << "Couldn't set up XKB." << std::endl;
		return;
		}
}


XCBCairoApp::~XCBCairoApp()
{
	// Clean up.
	keyboard.close();
	xcb_connection.disconnect();
}


CairoWindow* XCBCairoApp::new_window()
{
	auto window = new XCBCairoWindow();
	windows_by_id[window->x_window] = window;
	return window;
}


void XCBCairoApp::run()
{
	// Handle the first XCB events if any are ready, they might include the
	// resize event.
	while (true) {
		auto event = xcb_poll_for_event(xcb_connection.connection);
		if (event == nullptr)
			break;
		handle_x11_event(event);
		}

	// Event loop.
	std::vector<struct pollfd> poll_fds = {
		{ xcb_get_file_descriptor(xcb_connection.connection), POLLIN, 0 },
		{ signal.poll_fd(), POLLIN, 0 },
		};
	running = true;
	while (running) {
		// Redraw any windows that need it.
		for (auto& it: windows_by_id) {
			auto window = it.second;
			if (window->redraw_requested) {
				window->redraw();
				window->redraw_requested = false;
				}
			}

		if (fd_update_needed) {
			poll_fds.resize(1);
			for (auto& it: client_fds)
				poll_fds.push_back({ it.first, POLLIN, 0 });
			}

		// xcb_poll_for_event() is tricky!  It can have an event for us even if
		// poll() wouldn't see any input on its fd.  Maybe this arises during
		// other XCB calls?  Anyway, we'll check for any event even *before*
		// calling poll() (and again later if we don't have one now).
		auto event = xcb_poll_for_event(xcb_connection.connection);

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
		if (num_waiting <= 0 && event == nullptr)
			continue;

		// Client fds.
		for (int i = 2; i < (int) poll_fds.size(); ++i) {
			if ((poll_fds[i].revents & POLLIN) != 0) {
				auto it = client_fds.find(poll_fds[i].fd);
				if (it != client_fds.end())
					it->second();
				}
			}

		// Refresh signal.
		if ((poll_fds[1].revents & POLLIN) != 0) {
			// Consolidate multiple refresh requests.
			struct pollfd just_signal_poll_fd = { signal.poll_fd(), POLLIN, 0 };
			while (poll(&just_signal_poll_fd, 1, 0) > 0) {
				auto message = signal.message();
				if (message == 'Q')
					running = false;
				}
			}

		if (event == nullptr)
			event = xcb_poll_for_event(xcb_connection.connection);
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
}


XCBCairoWindow* XCBCairoApp::window_for(xcb_window_t x_window)
{
	auto it = windows_by_id.find(x_window);
	if (it == windows_by_id.end())
		return nullptr;
	return it->second;
}

void XCBCairoApp::handle_x11_event(xcb_generic_event_t* event)
{
	auto event_type = event->response_type & ~0x80;
	switch (event_type) {
		case XCB_EXPOSE:
			{
			auto window = window_for(((xcb_expose_event_t*) event)->window);
			if (window)
				window->redraw_requested = true;
			}
			break;

		case XCB_BUTTON_PRESS:
			{
			auto button_event = (xcb_button_press_event_t*) event;
			auto window = window_for(button_event->event);
			if (window) {
				window->mouse_pressed(button_event->event_x, button_event->event_y, button_event->detail);
				window->redraw_requested = true;
				}
			}
			break;
		case XCB_BUTTON_RELEASE:
			{
			auto button_event = (xcb_button_release_event_t*) event;
			auto window = window_for(button_event->event);
			if (window) {
				window->mouse_released(button_event->event_x, button_event->event_y, button_event->detail);
				window->redraw_requested = true;
				}
			}
		case XCB_MOTION_NOTIFY:
			{
			auto motion_event = (xcb_motion_notify_event_t*) event;
			auto window = window_for(motion_event->event);
			if (window) {
				window->mouse_moved(motion_event->event_x, motion_event->event_y);
				window->redraw_requested = true;
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
				window->resized(configure_event->width, configure_event->height);
				window->redraw_requested = true;
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

void XCBCairoApp::handle_key_event(xcb_key_press_event_t* event)
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
		if (codepoint) {
			if ((event->state & XCB_MOD_MASK_1) != 0) {
				// Alt-<something>.
				//*** TODO
				}
			else
				window->key_pressed(codepoint);
			}
		}
	window->redraw_requested = true;
}


