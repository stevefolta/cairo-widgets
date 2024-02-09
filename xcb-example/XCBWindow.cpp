#include "XCBWindow.h"
#include <cairo/cairo-xcb.h>

double XCBWindow::default_width = 700;
double XCBWindow::default_height = 400;
Color XCBWindow::default_background_color = { 1.0, 1.0, 1.0 };


XCBWindow::XCBWindow(double initial_width, double initial_height)
	: CompoundWidget(&cairo_gui, { 0, 0, initial_width, initial_height }), cairo_gui(this)
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
		0, 0, rect.width, rect.height, 1,
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
	surface = cairo_xcb_surface_create(xcb_connection.connection, x_window, visual, rect.width, rect.height);
	cairo = cairo_create(surface);
}


XCBWindow::~XCBWindow()
{
	if (cairo)
		cairo_destroy(cairo);
	if (surface)
		cairo_surface_destroy(surface);
	if (x_window)
		xcb_destroy_window(xcb_connection.connection, x_window);
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


void XCBWindow::resize(double new_width, double new_height)
{
	rect.width = new_width;
	rect.height = new_height;
	cairo_xcb_surface_set_size(surface, rect.width, rect.height);
	layout();
}


void XCBWindow::mouse_pressed(int32_t x, int32_t y, int button)
{
	if (button != 1)
		return;

	CompoundWidget::mouse_pressed(x, y);
}

void XCBWindow::mouse_released(int32_t x, int32_t y, int button)
{
	if (button == 1)
		CompoundWidget::mouse_released(x, y);
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



