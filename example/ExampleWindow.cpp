#include "ExampleWindow.h"
#include "Button.h"
#include "CairoGUI.h"
#include <iostream>

static const double margin = 10.0;
static const double default_button_width = 80.0;
static const double default_button_height = 24.0;
static const Color background_color = { 1.0, 1.0, 1.0 };


ExampleWindow::ExampleWindow(CairoGUI* cairo_gui_in)
	: cairo_gui(cairo_gui_in)
{
	button = new Button(cairo_gui, "OK");
}


ExampleWindow::~ExampleWindow()
{
	delete button;
}


void ExampleWindow::paint()
{
	// Draw the background.
	auto cairo = cairo_gui->cairo();
	cairo_save(cairo);
	cairo_rectangle(cairo, 0, 0, width, height);
	cairo_set_source_rgb(cairo, background_color.red, background_color.green, background_color.blue);
	cairo_fill(cairo);
	cairo_restore(cairo);

	button->paint();
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
	auto button_width = default_button_width;
	auto button_height = default_button_height;
	if (height > 1000) {
		button_width *= 2;
		button_height *= 2;
		}
	button->rect = { margin, margin, button_width, button_height };
}


