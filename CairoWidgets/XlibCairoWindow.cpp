#include "XlibCairoWindow.h"
#include "CompoundWidget.h"
#include <X11/cursorfont.h>
#include <cairo/cairo-xlib.h>


XlibCairoWindow::XlibCairoWindow(
	Display* x_display_in, Atom wm_delete_window_atom_in,
	double initial_width, double initial_height
	)
	: CairoWindow(initial_width, initial_height),
	display(x_display_in), wm_delete_window_atom(wm_delete_window_atom_in),
	cairo_gui(this)
{
	XSetWindowAttributes attributes = {};
	x_window =
		XCreateWindow(
			display, DefaultRootWindow(display),
			0, 0, width, height,
			0, 0, InputOutput, CopyFromParent,
			CWOverrideRedirect, &attributes);
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

	XSetWMProtocols(display, x_window, &wm_delete_window_atom, 1);

	// Create the Cairo stuff.
	surface = cairo_xlib_surface_create(display, x_window, DefaultVisual(display, 0), width, height);
	cairo = cairo_create(surface);
}

XlibCairoWindow::~XlibCairoWindow()
{
	if (cairo)
		cairo_destroy(cairo);
	if (surface)
		cairo_surface_destroy(surface);
	XDestroyWindow(display, x_window);
}

bool XlibCairoWindow::is_valid()
{
	return x_window != 0 && cairo != nullptr;
}


void XlibCairoWindow::resized(double new_width, double new_height)
{
	width = new_width;
	height = new_height;
	cairo_xlib_surface_set_size(surface, width, height);
	if (widget) {
		widget->rect = { 0, 0, width, height };
		widget->layout();
		}
}


void XlibCairoWindow::set_title(const std::string& title)
{
	XStoreName(display, x_window, title.c_str());
}



