#include "ScrollArea.h"
#include "Scrollbar.h"
#include "CairoGUI.h"

ScrollArea::Style ScrollArea::default_style;


ScrollArea::ScrollArea(CairoGUI* gui_in, Widget* contents_in, Rect rect_in)
	: Widget(gui_in, rect_in), contents(contents_in), adjusted_cairo_gui(this)
{
	scrollbar = new Scrollbar(gui, scrollbar_rect());
}


ScrollArea::~ScrollArea()
{
	delete scrollbar;
	delete contents;
}


void ScrollArea::paint()
{
	update_scrollbar();

	// Clip.
	auto cairo = gui->cairo();
	cairo_save(cairo);
	cairo_rectangle(cairo, rect.x, rect.y, rect.width, rect.height);
	cairo_clip(cairo);

	// Contents.
	if (contents) {
		cairo_save(cairo);
		cairo_translate(cairo, rect.x - contents->rect.x, rect.y - contents->rect.y -scrollbar->value);
		contents->paint();
		cairo_restore(cairo);
		}

	if (style.always_show_scrollbar || scrollbar->percentage < 1.0)
		scrollbar->paint();

	cairo_restore(cairo);

	// Border.
	use_rect(rect);
	use_color(style.border_color);
	cairo_set_line_width(cairo, style.border_width);
	cairo_stroke(cairo);

	// Foreground.
	if (contents) {
		cairo_save(cairo);
		cairo_translate(cairo, rect.x - contents->rect.x, rect.y - contents->rect.y - scrollbar->value);
		paint_foreground();
		cairo_restore(cairo);
		}
}


void ScrollArea::mouse_pressed(int x, int y)
{
	// Check for a "tapped open" widget.
	if (tracking_widget && tracking_widget == contents) {
		auto adjusted_x = x - rect.x + contents->rect.x;
		auto adjusted_y = y - rect.y + contents->rect.y + scrollbar->value;
		if (tracking_widget->contains(x, y)) {
			tracking_widget->mouse_pressed(adjusted_x, adjusted_y);
			return;
			}
		else {
			tracking_widget->mouse_released(adjusted_x, adjusted_y);
			tracking_widget = nullptr;
			}
		}

	update_scrollbar();
	if (scrollbar->contains(x, y)) {
		tracking_widget = scrollbar;
		tracking_widget->mouse_pressed(x, y);
		}
	else if (contents) {
		auto adjusted_x = x - rect.x + contents->rect.x;
		auto adjusted_y = y - rect.y + contents->rect.y + scrollbar->value;
		if (contents->contains(adjusted_x, adjusted_y)) {
			tracking_widget = contents;
			tracking_widget->mouse_pressed(adjusted_x, adjusted_y);
			return;
			}
		}
	if (tracking_widget)
		tracking_widget->mouse_pressed(x, y);
}

bool ScrollArea::mouse_released(int x, int y)
{
	update_scrollbar();
	bool accepted = false;
	if (tracking_widget == scrollbar) {
		accepted = scrollbar->mouse_released(x, y);
		tracking_widget = nullptr;
		}
	else if (contents && tracking_widget == contents) {
		auto adjusted_x = x - rect.x + contents->rect.x;
		auto adjusted_y = y - rect.y + contents->rect.y + scrollbar->value;
		accepted = contents->mouse_released(adjusted_x, adjusted_y);
		if (!tracking_widget->sticky_tracking())
			tracking_widget = nullptr;
		}
	else
		tracking_widget = nullptr;
	return accepted;
}

void ScrollArea::mouse_moved(int x, int y)
{
	// We won't call update_scrollbar() here; it should suffice that we do it in
	// paint().

	if (contents) {
		auto adjusted_x = x - rect.x + contents->rect.x;
		auto adjusted_y = y - rect.y + contents->rect.y + scrollbar->value;
		contents->mouse_moved(adjusted_x, adjusted_y);
		}
	scrollbar->mouse_moved(x, y);
}

bool ScrollArea::sticky_tracking()
{
	return (tracking_widget != nullptr);
}


void ScrollArea::scroll_down(int x, int y)
{
	auto wheel_scroll_amount = style.relative_wheel_scroll_amount * rect.height;
	scrollbar->value += wheel_scroll_amount;
	double max_value = scrollbar->max * (1 - scrollbar->percentage);
	if (scrollbar->value > max_value)
		scrollbar->value = max_value;
}

void ScrollArea::scroll_up(int x, int y)
{
	auto wheel_scroll_amount = style.relative_wheel_scroll_amount * rect.height;
	scrollbar->value -= wheel_scroll_amount;
	if (scrollbar->value < 0.0)
		scrollbar->value = 0.0;
}


void ScrollArea::update_scrollbar()
{
	scrollbar->rect = scrollbar_rect();
	if (contents == nullptr || contents->rect.height <= 0) {
		scrollbar->max = 0.0;
		scrollbar->percentage = 1.0;
		scrollbar->value = 0.0;
		}
	else {
		scrollbar->max = contents->rect.height;
		scrollbar->percentage = rect.height / scrollbar->max;
		if (scrollbar->percentage > 1.0)
			scrollbar->percentage = 1.0;
		auto max_scroll_value = scrollbar->max * (1 - scrollbar->percentage);
		if (scrollbar->value > max_scroll_value)
			scrollbar->value = max_scroll_value;
		}
}

Rect ScrollArea::scrollbar_rect()
{
	return {
		rect.x + rect.width - style.scrollbar_inset - style.scrollbar_width,
		rect.y + style.scrollbar_inset,
		style.scrollbar_width,
		rect.height - 2 * style.scrollbar_inset,
		};
}


Rect ScrollArea::ScrolledCairoGUI::popup_limits()
{
	Rect limits = scroll_area->gui->popup_limits();
	if (limits.width <= 0 || limits.height <= 0)
		return limits;
	limits.x -= scroll_area->rect.x;
	limits.y -= scroll_area->rect.y - scroll_area->scrollbar->value;
	if (scroll_area->contents) {
		limits.x += scroll_area->contents->rect.x;
		limits.y += scroll_area->contents->rect.y;
		}
	return limits;
}



