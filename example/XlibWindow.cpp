#include "XlibWindow.h"
#include <X11/cursorfont.h>
#include <cairo/cairo-xlib.h>

double XlibWindow::default_width = 700;
double XlibWindow::default_height = 400;
Color XlibWindow::default_background_color = { 1.0, 1.0, 1.0 };


XlibWindow::XlibWindow(Display* x_display, Atom wm_delete_window_atom_in, double initial_width, double initial_height)
	: CompoundWidget(&cairo_gui, { 0, 0, initial_width, initial_height }),
	  display(x_display), wm_delete_window_atom(wm_delete_window_atom_in),
		cairo_gui(this)
{
	XSetWindowAttributes attributes = {};
	x_window =
		XCreateWindow(
			display, DefaultRootWindow(display),
			0, 0, rect.width, rect.height,
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
	surface = cairo_xlib_surface_create(display, x_window, DefaultVisual(display, 0), rect.width, rect.height);
	cairo = cairo_create(surface);
}

XlibWindow::~XlibWindow()
{
	if (cairo)
		cairo_destroy(cairo);
	if (surface)
		cairo_surface_destroy(surface);
	XDestroyWindow(display, x_window);
}


void XlibWindow::redraw()
{
	paint();
	cairo_gui.refresh();
}

void XlibWindow::paint()
{
	auto cairo = cairo_gui.cairo();
	cairo_push_group(cairo);

	// Draw the background.
	cairo_save(cairo);
	use_rect(rect);
	use_color(background_color);
	cairo_fill(cairo);
	cairo_restore(cairo);

	// Draw the widgets.
	CompoundWidget::paint();

	// Blit to screen.
	cairo_pop_group_to_source(cairo);
	cairo_paint(cairo);
}


void XlibWindow::resize(double new_width, double new_height)
{
	rect.width = new_width;
	rect.height = new_height;
	cairo_xlib_surface_set_size(surface, rect.width, rect.height);
	layout();
}

void XlibWindow::mouse_pressed(int32_t x, int32_t y, int button)
{
	if (button == Button4 || button == Button5) {
		for (auto widget: all_widgets) {
			if (widget->contains(x, y)) {
				if (button == Button5)
					widget->scroll_down(x, y);
				else
					widget->scroll_up(x, y);
				widget->mouse_moved(x, y);
				break;
				}
			}
		return;
		}

	else if (button != Button1)
		return;

	CompoundWidget::mouse_pressed(x, y);
}

void XlibWindow::mouse_released(int32_t x, int32_t y, int button)
{
	if (button == Button1)
		CompoundWidget::mouse_released(x, y);
}


void XlibWindow::key_pressed(int c)
{
	if (focused_widget)
		focused_widget->key_pressed(c);
}

void XlibWindow::special_key_pressed(SpecialKey key)
{
	if (focused_widget)
		focused_widget->special_key_pressed(key);
}


void XlibWindow::set_title(const std::string& title)
{
	XStoreName(display, x_window, title.c_str());
}


