#include "ExampleWindow.h"
#include "CairoGUI.h"
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>
#include <cairo/cairo-xlib.h>
#include <poll.h>
#include <map>
#include <iostream>

Display* display = nullptr;
int screen = 0;
Window x_window = 0;
Atom wm_delete_window_atom = 0;
cairo_surface_t* surface = nullptr;
cairo_t* cairo = nullptr;
ExampleWindow* example_window = nullptr;
bool running = true;

class ExampleCairoGUI : public CairoGUI {
	public:
		cairo_t* cairo() { return ::cairo; }
		void refresh() {
			if (surface) {
				cairo_surface_flush(surface);
				XFlush(display);
				}
			};
	};
static ExampleCairoGUI cairo_gui;

extern void paint();
extern void redraw();
extern void handle_x11_event(XEvent* event);
extern void handle_key_event(XKeyEvent* event);


int fail(const char* message)
{
	std::cerr << message << std::endl;
	return 1;
}

int main(int argc, const char* argv[])
{
	static int width = 700;
	static int height = 400;

	// Create the X window.
	display = XOpenDisplay(nullptr);
	if (display == nullptr)
		return fail("Can't open display");
	screen = XDefaultScreen(display);
	XSetWindowAttributes attributes = {};
	x_window =
		XCreateWindow(
			display, DefaultRootWindow(display),
			0, 0, width, height,
			0, 0, InputOutput, CopyFromParent,
			CWOverrideRedirect, &attributes);
	XStoreName(display, x_window, "CairoWidgets");
	wm_delete_window_atom = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, x_window, &wm_delete_window_atom, 1);
	XSelectInput(
		display, x_window,
		SubstructureNotifyMask | ExposureMask | PointerMotionMask |
			ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask |
			StructureNotifyMask | EnterWindowMask | LeaveWindowMask | ButtonMotionMask |
			KeymapStateMask | FocusChangeMask | PropertyChangeMask);
	// Use the "default" cursor.  "Default" is ill-defined here, XC_left_ptr
	// matches what most programs show.
	Cursor cursor = XCreateFontCursor(display, XC_left_ptr);
	XDefineCursor(display, x_window, cursor);
	XMapRaised(display, x_window);

	// Create the Cairo stuff.
	surface = cairo_xlib_surface_create(display, x_window, DefaultVisual(display, 0), width, height);
	cairo = cairo_create(surface);

	example_window = new ExampleWindow(&cairo_gui);
	example_window->resize(width, height);

	// Event loop.
	std::vector<struct pollfd> poll_fds = {
		{ ConnectionNumber(display), POLLIN, 0 },
		};
	while (running) {
		// Wait for X event, or timeout.
		int timeout_ms = example_window->next_update_ms();
		int num_waiting = poll(poll_fds.data(), poll_fds.size(), timeout_ms);
		if (num_waiting <= 0) {
			paint();
			cairo_gui.refresh();
			continue;
			}

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
		paint();
		cairo_gui.refresh();
		}

	// Clean up.
	delete example_window;
	if (cairo) {
		cairo_destroy(cairo);
		cairo = nullptr;
		}
	if (surface) {
		cairo_surface_destroy(surface);
		surface = nullptr;
		}
	if (display) {
		XDestroyWindow(display, x_window);
		XCloseDisplay(display);
		display = nullptr;
		}

	return 0;
}


void paint()
{
	example_window->paint();
}


void redraw()
{
	paint();
	cairo_gui.refresh();
}


void handle_x11_event(XEvent* event)
{
	switch (event->type) {
		case Expose:
			if (event->xexpose.window == x_window)
				redraw();
			break;

		case ButtonPress:
			if (event->xbutton.window == x_window)
				example_window->mouse_pressed(event->xbutton.x, event->xbutton.y, event->xbutton.button);
			break;
		case ButtonRelease:
			if (event->xbutton.window == x_window)
				example_window->mouse_released(event->xbutton.x, event->xbutton.y, event->xbutton.button);
			break;
		case MotionNotify:
			if (event->xmotion.window == x_window)
				example_window->mouse_moved(event->xmotion.x, event->xmotion.y);
			break;
		case KeyPress:
			if (event->xkey.window == x_window)
				handle_key_event(&event->xkey);
			break;
		case ConfigureNotify:
			if (event->xconfigure.window == x_window) {
				cairo_xlib_surface_set_size(surface, event->xconfigure.width, event->xconfigure.height);
				example_window->resize(event->xconfigure.width, event->xconfigure.height);
				redraw();
				}
			break;
		case ClientMessage:
			if ((Atom) event->xclient.data.l[0] == wm_delete_window_atom)
				running = false;
			break;
		}
}


static const std::map<KeySym, SpecialKey> special_keys = {
	{ XK_Left, LeftArrow }, { XK_Right, RightArrow }, { XK_Up, UpArrow }, { XK_Down, DownArrow },
	{ XK_Page_Up, PageUp }, { XK_Page_Down, PageDown }, { XK_Home, HomeKey }, { XK_End, EndKey },
	};

void handle_key_event(XKeyEvent* event)
{
	char buffer[64];
	KeySym key_sym;
	int num_chars = XLookupString(event, buffer, sizeof(buffer), &key_sym, nullptr);
	if (num_chars > 0) {
		for (int i = 0; i < num_chars; ++i)
			example_window->key_pressed(buffer[i]);
		}
	else {
		auto it = special_keys.find(key_sym);
		if (it != special_keys.end())
			example_window->special_key_pressed(it->second);
		}
}



