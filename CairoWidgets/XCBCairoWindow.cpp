#include "XCBCairoWindow.h"
#include "CompoundWidget.h"
#include <cairo/cairo-xcb.h>


XCBCairoWindow::XCBCairoWindow(double initial_width, double initial_height)
	: CairoWindow(initial_width, initial_height), cairo_gui(this)
{
	// Create the window.
	x_window = xcb_generate_id(xcb_connection.connection);
	uint32_t values[] = {
		XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS |
			XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
			XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY,
		};
	xcb_create_window(
		xcb_connection.connection, xcb_connection.screen->root_depth, x_window, xcb_connection.screen->root,
		0, 0, width, height, 1,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		xcb_connection.screen->root_visual,
		XCB_CW_EVENT_MASK, values);
	xcb_map_window(xcb_connection.connection, x_window);

	// Set up with the window manager.
	auto delete_atom = xcb_connection.atom("WM_DELETE_WINDOW");
	xcb_change_property(
		xcb_connection.connection, XCB_PROP_MODE_REPLACE, x_window,
		xcb_connection.atom("WM_PROTOCOLS"), XCB_ATOM_ATOM, 32 /* bits */,
		1, &delete_atom);

	xcb_flush(xcb_connection.connection);

	// Create the Cairo stuff.
	auto visual = xcb_connection.find_visual(xcb_connection.screen->root_visual);
	if (visual == nullptr)
		return;
	surface = cairo_xcb_surface_create(xcb_connection.connection, x_window, visual, width, height);
	cairo = cairo_create(surface);
}


XCBCairoWindow::~XCBCairoWindow()
{
	if (cairo)
		cairo_destroy(cairo);
	if (surface)
		cairo_surface_destroy(surface);
	if (x_window)
		xcb_destroy_window(xcb_connection.connection, x_window);
}


void XCBCairoWindow::resized(double new_width, double new_height)
{
	width = new_width;
	height = new_height;
	cairo_xcb_surface_set_size(surface, width, height);
	if (widget) {
		widget->rect = { 0, 0, width, height };
		widget->layout();
		}
}


void XCBCairoWindow::set_title(const std::string& title)
{
	xcb_change_property(
		xcb_connection.connection, XCB_PROP_MODE_REPLACE, x_window,
		xcb_connection.atom("WM_NAME"), xcb_connection.atom("UTF8_STRING"), 8 /* bits */,
		title.size(), title.data());
}



