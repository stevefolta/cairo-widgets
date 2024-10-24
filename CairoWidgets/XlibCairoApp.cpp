#include "XlibCairoApp.h"
#include "XlibCairoWindow.h"
#include "Widget.h"
#include <X11/Xutil.h>
#include <poll.h>
#include <iostream>


XlibCairoApp::XlibCairoApp()
{
	display = XOpenDisplay(nullptr);
	if (display == nullptr) {
		std::cerr << "Can't open display" << std::endl;
		return;
		}
	wm_delete_window_atom = XInternAtom(display, "WM_DELETE_WINDOW", False);
}

XlibCairoApp::~XlibCairoApp()
{
	if (display)
		XCloseDisplay(display);
}


void XlibCairoApp::run()
{
	running = true;

	// Event loop.
	std::vector<struct pollfd> poll_fds = {
		{ ConnectionNumber(display), POLLIN, 0 },
		{ signal.poll_fd(), POLLIN, 0 },
		};
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
			poll_fds.resize(2);
			for (auto& it: client_fds)
				poll_fds.push_back({ it.first, POLLIN, 0 });
			fd_update_needed = false;
			}

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
				window->redraw_requested = true;
			}
		if (num_waiting <= 0)
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

		// X events.
		if (XPending(display)) {
			XEvent event;
			XNextEvent(display, &event);

			while (XPending(display)) {
				XEvent next_event;
				XNextEvent(display, &next_event);

				if (event.type == MotionNotify && next_event.type == MotionNotify) {
					// Merge adjacent mouse motion events.
					}
				else {
					handle_x11_event(&event);
					XFlush(display);
					}

				event = next_event;
				}

			handle_x11_event(&event);
			}

		XFlush(display);
		}
}


CairoWindow* XlibCairoApp::new_window()
{
	auto window = new XlibCairoWindow(display, wm_delete_window_atom);
	if (!window->is_valid()) {
		delete window;
		return nullptr;
		}
	windows_by_id[window->x_window] = window;
	return window;
}


XlibCairoWindow* XlibCairoApp::window_for(Window x_window)
{
	auto it = windows_by_id.find(x_window);
	if (it == windows_by_id.end())
		return nullptr;
	return it->second;
}

void XlibCairoApp::handle_x11_event(XEvent* event)
{
	switch (event->type) {
		case Expose:
			{
			auto window = window_for(event->xexpose.window);
			if (window)
				window->redraw_requested = true;
			}
			break;

		case ButtonPress:
			{
			auto window = window_for(event->xbutton.window);
			if (window) {
				window->mouse_pressed(event->xbutton.x, event->xbutton.y, event->xbutton.button);
				window->redraw_requested = true;
				}
			}
			break;
		case ButtonRelease:
			{
			auto window = window_for(event->xbutton.window);
			if (window) {
				window->mouse_released(event->xbutton.x, event->xbutton.y, event->xbutton.button);
				window->redraw_requested = true;
				}
			}
			break;
		case MotionNotify:
			{
			auto window = window_for(event->xmotion.window);
			if (window) {
				window->mouse_moved(event->xmotion.x, event->xmotion.y);
				window->redraw_requested = true;
				}
			}
			break;
		case KeyPress:
			handle_key_event(&event->xkey);
			break;
		case ConfigureNotify:
			{
			auto window = window_for(event->xconfigure.window);
			if (window) {
				window->resized(event->xconfigure.width, event->xconfigure.height);
				window->redraw_requested = true;
				}
			}
			break;
		case ClientMessage:
			{
			if ((Atom) event->xclient.data.l[0] == wm_delete_window_atom) {
				auto window = window_for(event->xclient.window);
				if (window) {
					windows_by_id.erase(window->x_window);
					delete window;
					}
				if (windows_by_id.empty())
					running = false;
				}
			}
			break;
		}
}


static const std::map<KeySym, SpecialKey> special_keys = {
	{ XK_Left, LeftArrow }, { XK_Right, RightArrow }, { XK_Up, UpArrow }, { XK_Down, DownArrow },
	{ XK_Page_Up, PageUp }, { XK_Page_Down, PageDown }, { XK_Home, HomeKey }, { XK_End, EndKey },
	};

void XlibCairoApp::handle_key_event(XKeyEvent* event)
{
	auto window = window_for(event->window);
	if (window == nullptr)
		return;

	char buffer[64];
	KeySym key_sym;
	int num_chars = XLookupString(event, buffer, sizeof(buffer), &key_sym, nullptr);
	if (num_chars > 0) {
		for (int i = 0; i < num_chars; ++i)
			window->key_pressed(buffer[i]);
		}
	else {
		auto it = special_keys.find(key_sym);
		if (it != special_keys.end())
			window->special_key_pressed(it->second);
		}
	window->redraw_requested = true;
}



