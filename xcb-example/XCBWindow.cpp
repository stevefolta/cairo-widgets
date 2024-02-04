#include "XCBWindow.h"
#include <cairo/cairo-xcb.h>

double XCBWindow::default_width = 700;
double XCBWindow::default_height = 400;
Color XCBWindow::default_background_color = { 1.0, 1.0, 1.0 };


XCBWindow::XCBWindow(double initial_width, double initial_height)
	: cairo_gui(this), width(initial_width), height(initial_height)
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


XCBWindow::~XCBWindow()
{
	for (auto widget: all_widgets)
		delete widget;

	if (cairo)
		cairo_destroy(cairo);
	if (surface)
		cairo_surface_destroy(surface);
}


void XCBWindow::redraw()
{
	paint();
	cairo_gui.refresh();
}

void XCBWindow::paint()
{
	auto cairo = cairo_gui.cairo();
	cairo_push_group(cairo);

	// Draw the background.
	cairo_save(cairo);
	cairo_rectangle(cairo, 0, 0, width, height);
	cairo_set_source_rgb(cairo, background_color.red, background_color.green, background_color.blue);
	cairo_fill(cairo);
	cairo_restore(cairo);

	// Always draw the tracking_widget on top.  For example, it could be a
	// popped-up PopupMenu.
	for (auto widget: all_widgets) {
		if (widget != tracking_widget)
			widget->paint();
		}
	if (tracking_widget)
		tracking_widget->paint();

	// Blit to screen.
	cairo_pop_group_to_source(cairo);
	cairo_paint(cairo);
}


void XCBWindow::resize(double new_width, double new_height)
{
	width = new_width;
	height = new_height;
	cairo_xcb_surface_set_size(surface, width, height);
	layout();
}


void XCBWindow::mouse_pressed(int32_t x, int32_t y, int button)
{
	if (button != 1)
		return;

	tracking_widget = nullptr;
	for (auto widget: all_widgets) {
		if (widget->contains(x, y)) {
			tracking_widget = widget;
			break;
			}
		}
	if (tracking_widget)
		tracking_widget->mouse_pressed(x, y);
}

void XCBWindow::mouse_released(int32_t x, int32_t y, int button)
{
	if (tracking_widget) {
		if (tracking_widget->mouse_released(x, y))
			widget_accepted(tracking_widget);
		}
	tracking_widget = nullptr;
}

void XCBWindow::mouse_moved(int32_t x, int32_t y)
{
	for (auto widget: all_widgets)
		widget->mouse_moved(x, y);
}


void XCBWindow::key_pressed(int c)
{
	if (focused_widget)
		focused_widget->key_pressed(c);
}

void XCBWindow::special_key_pressed(SpecialKey key)
{
	if (focused_widget)
		focused_widget->special_key_pressed(key);
}


void XCBWindow::set_title(const std::string& title)
{
	xcb_change_property(
		xcb_connection.connection, XCB_PROP_MODE_REPLACE, x_window,
		xcb_connection.atom("WM_NAME"), xcb_connection.atom("UTF8_STRING"), 8 /* bits */,
		title.size(), title.data());
}



