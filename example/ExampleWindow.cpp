#include "ExampleWindow.h"
#include "Button.h"
#include "SimplePopupMenu.h"
#include "CairoGUI.h"
#include <iostream>

static const double margin = 10.0;
static const double default_button_width = 80.0;
static const double default_button_height = 24.0;
static const double default_menu_width = 120.0;
static const double default_menu_height = 24.0;
static const double default_spacing = 6.0;
static const Color background_color = { 1.0, 1.0, 1.0 };


ExampleWindow::ExampleWindow(CairoGUI* cairo_gui_in)
	: cairo_gui(cairo_gui_in)
{
	button = new Button(cairo_gui, "OK");
	menu = new SimplePopupMenu(cairo_gui, { "Yes", "No", "Maybe" });
	color_menu = new SimplePopupMenu(cairo_gui, { "Red", "Green", "Blue" });
}


ExampleWindow::~ExampleWindow()
{
	delete button;
	delete menu;
	delete color_menu;
}


void ExampleWindow::paint()
{
	auto cairo = cairo_gui->cairo();
	cairo_push_group(cairo);

	// Draw the background.
	cairo_save(cairo);
	cairo_rectangle(cairo, 0, 0, width, height);
	cairo_set_source_rgb(cairo, background_color.red, background_color.green, background_color.blue);
	cairo_fill(cairo);
	cairo_restore(cairo);

	// Draw widgets, always drawing a popped-up menu on top.
	button->paint();
	if (tracking_widget != menu)
		menu->paint();
	if (tracking_widget != color_menu)
		color_menu->paint();
	if (tracking_widget == menu || tracking_widget == color_menu)
		tracking_widget->paint();

	// Blit to screen.
	cairo_pop_group_to_source(cairo);
	cairo_paint(cairo);
}


void ExampleWindow::resize(double new_width, double new_height)
{
	width = new_width;
	height = new_height;
	layout();
}


void ExampleWindow::mouse_pressed(int32_t x, int32_t y, int button)
{
	if (button != 1)
		return;

	if (this->button->contains(x, y))
		tracking_widget = this->button;
	else if (menu->contains(x, y))
		tracking_widget = menu;
	else if (color_menu->contains(x, y))
		tracking_widget = color_menu;
	if (tracking_widget)
		tracking_widget->mouse_pressed(x, y);
}

void ExampleWindow::mouse_released(int32_t x, int32_t y, int button)
{
	if (button != 1)
		return;

	if (tracking_widget) {
		tracking_widget->mouse_released(x, y);
		tracking_widget = nullptr;
		}
}

void ExampleWindow::mouse_moved(int32_t x, int32_t y)
{
	if (tracking_widget)
		tracking_widget->mouse_moved(x, y);
}


void ExampleWindow::layout()
{
	auto spacing = default_spacing;
	auto button_width = default_button_width;
	auto button_height = default_button_height;
	auto menu_width = default_menu_width;
	auto menu_height = default_menu_height;
	if (height > 1000) {
		spacing *= 2;
		button_width *= 2;
		button_height *= 2;
		menu_width *= 2;
		menu_height *= 2;
		menu->margin = 12.0;
		color_menu->margin = 12.0;
		}
	menu->rect = { margin, margin, menu_width, menu_height };
	auto top = margin + menu_height + spacing;
	button->rect = { margin, top, button_width, button_height };
	top += button_height + spacing;
	color_menu->rect = { margin, top, menu_width, menu_height };
}


