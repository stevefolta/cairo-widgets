#include "HorizontalSlider.h"
#include "CairoGUI.h"
#include <cairo/cairo.h>


void HorizontalSlider::paint()
{
	auto cairo = gui->cairo();
	cairo_save(cairo);

	// Track.
	cairo_rectangle(cairo, rect.x, rect.y + (rect.height - track_height) / 2, rect.width, track_height);
	cairo_set_source_rgb(cairo, track_color.red, track_color.green, track_color.blue);
	cairo_fill(cairo);

	// Thumb.
	double thumb_x = (value - min_value) * (rect.width - thumb_width) / (max_value - min_value);
	Rect thumb_rect = { rect.x + thumb_x, rect.y, thumb_width, rect.height };
	rounded_rect(thumb_rect, thumb_width / 4.0);
	cairo_set_source_rgb(cairo, thumb_fill_color.red, thumb_fill_color.green, thumb_fill_color.blue);
	cairo_fill_preserve(cairo);
	cairo_set_source_rgb(cairo, thumb_stroke_color.red, thumb_stroke_color.green, thumb_stroke_color.blue);
	cairo_set_line_width(cairo, thumb_line_width);
	cairo_stroke(cairo);

	// Thumb crease.
	cairo_move_to(cairo, thumb_rect.x + thumb_rect.width / 2, thumb_rect.y + crease_inset);
	cairo_rel_line_to(cairo, 0, thumb_rect.height - 2 * crease_inset);
	cairo_set_source_rgb(cairo, thumb_crease_color.red, thumb_crease_color.green, thumb_crease_color.blue);
	cairo_set_line_width(cairo, crease_width);
	cairo_stroke(cairo);

	cairo_restore(cairo);
}


void HorizontalSlider::mouse_pressed(int x, int y)
{
	// Paging.
	double thumb_x = (value - min_value) * (rect.width - thumb_width) / (max_value - min_value);
	thumb_x += rect.x;
	if (x < thumb_x) {
		value -= (max_value - min_value) * page_factor;
		if (value < min_value)
			value = min_value;
		return;
		}
	if (x > thumb_x + thumb_width) {
		value += (max_value - min_value) * page_factor;
		if (value > max_value)
			value = max_value;
		return;
		}

	// Grabbed the thumb.
	initial_value = value;
	drag_offset = x - thumb_x;
	pressed = true;
}

bool HorizontalSlider::mouse_released(int x, int y)
{
	mouse_moved(x, y);
	/* Use this to make it snap back to the initial value if not released inside the slider:
	if (!contains(x, y)) {
		value = initial_value;
		return false;
		}
	*/

	pressed = false;
	return true;
}

void HorizontalSlider::mouse_moved(int x, int y)
{
	if (!pressed)
		return;

	value = (x - drag_offset - rect.x) * (max_value - min_value) / (rect.width - thumb_width) + min_value;
	if (value < min_value)
		value = min_value;
	else if (value > max_value)
		value = max_value;
}


