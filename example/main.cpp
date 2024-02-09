#include "ExampleWindow.h"
#include "CairoGUI.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <poll.h>
#include <map>
#include <iostream>

Display* display = nullptr;
Atom wm_delete_window_atom = 0;
std::map<Window, XlibWindow*> windows_by_id;
bool running = true;

extern void handle_x11_event(XEvent* event);
extern void handle_key_event(XKeyEvent* event);


int fail(const char* message)
{
	std::cerr << message << std::endl;
	return 1;
}

int main(int argc, const char* argv[])
{
	// Create the X window.
	display = XOpenDisplay(nullptr);
	if (display == nullptr)
		return fail("Can't open display");
	wm_delete_window_atom = XInternAtom(display, "WM_DELETE_WINDOW", False);

	ExampleWindow* example_window = new ExampleWindow(display, wm_delete_window_atom);
	example_window->set_title("CairoWidgets Example");
	windows_by_id[example_window->x_window] = example_window;

	// Event loop.
	std::vector<struct pollfd> poll_fds = {
		{ ConnectionNumber(display), POLLIN, 0 },
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
		XFlush(display);
		}

	// Clean up.
	XCloseDisplay(display);

	return 0;
}


static XlibWindow* window_for(Window x_window)
{
	auto it = windows_by_id.find(x_window);
	if (it == windows_by_id.end())
		return nullptr;
	return it->second;
}

void handle_x11_event(XEvent* event)
{
	switch (event->type) {
		case Expose:
			{
			auto window = window_for(event->xexpose.window);
			if (window)
				window->redraw();
			}
			break;

		case ButtonPress:
			{
			auto window = window_for(event->xbutton.window);
			if (window) {
				window->mouse_pressed(event->xbutton.x, event->xbutton.y, event->xbutton.button);
				window->redraw();
				}
			}
			break;
		case ButtonRelease:
			{
			auto window = window_for(event->xbutton.window);
			if (window) {
				window->mouse_released(event->xbutton.x, event->xbutton.y, event->xbutton.button);
				window->redraw();
				}
			}
			break;
		case MotionNotify:
			{
			auto window = window_for(event->xmotion.window);
			if (window) {
				window->mouse_moved(event->xmotion.x, event->xmotion.y);
				window->redraw();
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
				window->resize(event->xconfigure.width, event->xconfigure.height);
				window->redraw();
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

void handle_key_event(XKeyEvent* event)
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
	window->redraw();
}



