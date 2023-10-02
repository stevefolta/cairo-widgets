#include "ExampleWindow.h"
#include "Button.h"
#include "CairoGUI.h"
#include <iostream>

static const double margin = 10.0;
static const double button_width = 80.0;
static const double button_height = 20.0;
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


void ExampleWindow::layout()
{
	button->rect = { margin, margin, button_width, button_height };
}


