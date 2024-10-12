#include "CairoWindow.h"
#include "CompoundWidget.h"
#include "CairoGUI.h"


double CairoWindow::default_width = 700;
double CairoWindow::default_height = 400;
Color CairoWindow::default_background_color = { 1.0, 1.0, 1.0 };


CairoWindow::CairoWindow(double initial_width, double initial_height)
	: width(initial_width), height(initial_height)
{
}

CairoWindow::~CairoWindow()
{
	if (widget)
		delete widget;
}


void CairoWindow::set_widget(CompoundWidget* new_widget)
{
	if (widget)
		delete widget;
	widget = new_widget;
	if (widget) {
		widget->rect = { 0, 0, width, height };
		widget->layout();
		}
}

CompoundWidget* CairoWindow::replace_widget(CompoundWidget* new_widget)
{
	auto old_widget = widget;
	widget = new_widget;
	if (widget) {
		widget->rect = { 0, 0, width, height };
		widget->layout();
		}
	return old_widget;
}


void CairoWindow::redraw()
{
	paint();
	gui()->refresh();
}

void CairoWindow::paint()
{
	auto cairo = gui()->cairo();
	cairo_push_group(cairo);

	// Draw the background.
	cairo_save(cairo);
	cairo_rectangle(cairo, 0, 0, width, height);
	cairo_set_source_rgba(
		cairo,
		background_color.red, background_color.green, background_color.blue,
		background_color.alpha);
	cairo_fill(cairo);
	cairo_restore(cairo);

	// Draw the widget.
	if (widget)
		widget->paint();

	// Blit to screen.
	cairo_pop_group_to_source(cairo);
	cairo_paint(cairo);
}


void CairoWindow::mouse_pressed(int32_t x, int32_t y, int button)
{
	if (button == 4) {
		if (widget)
			widget->scroll_down(x, y);
		return;
		}
	else if (button == 5) {
		if (widget)
			widget->scroll_up(x, y);
		return;
		}

	else if (button != 1)
		return;

	if (widget)
		widget->mouse_pressed(x, y);
}

void CairoWindow::mouse_released(int32_t x, int32_t y, int button)
{
	if (button == 1 && widget)
		widget->mouse_released(x, y);
}

void CairoWindow::mouse_moved(int32_t x, int32_t y)
{
	if (widget)
		widget->mouse_moved(x, y);
}


void CairoWindow::key_pressed(int c)
{
	if (widget)
		widget->key_pressed(c);
}

void CairoWindow::special_key_pressed(SpecialKey key)
{
	if (widget)
		widget->special_key_pressed(key);
}


int CairoWindow::next_update_ms()
{
	return widget ? widget->next_update_ms() : -1;
}



