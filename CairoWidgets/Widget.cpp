#include "Widget.h"
#include "CairoGUI.h"
#include "CairoFontconfigFont.h"
#include <math.h>


bool Widget::contains(double x, double y)
{
	return rect.contains(x, y);
}


void Widget::rounded_rect(Rect rect, double corner_size)
{
	auto cairo = gui->cairo();

	cairo_new_sub_path(cairo);
	cairo_arc(cairo, rect.x + rect.width - corner_size, rect.y + corner_size, corner_size, -M_PI / 2.0, 0);
	cairo_arc(cairo, rect.x + rect.width - corner_size, rect.y + rect.height - corner_size, corner_size, 0, M_PI / 2.0);
	cairo_arc(cairo, rect.x + corner_size, rect.y + rect.height - corner_size, corner_size, M_PI / 2.0, M_PI);
	cairo_arc(cairo, rect.x + corner_size, rect.y + corner_size, corner_size, M_PI, 1.5 * M_PI);
	cairo_close_path(cairo);
}

void Widget::rounded_rect(Rect rect, double corner_width, double corner_height)
{
	auto cairo = gui->cairo();

	cairo_move_to(cairo, rect.x + corner_width, rect.y);
	cairo_line_to(cairo, rect.x + rect.width - corner_width, rect.y);
	cairo_curve_to(cairo,
		rect.x + rect.width, rect.y,
		rect.x + rect.width, rect.y,
		rect.x + rect.width, rect.y + corner_height);
	cairo_line_to(cairo, rect.x + rect.width, rect.y + rect.height - corner_height);
	cairo_curve_to(cairo,
		rect.x + rect.width, rect.y + rect.height,
		rect.x + rect.width, rect.y + rect.height,
		rect.x + rect.width - corner_width, rect.y + rect.height);
	cairo_line_to(cairo, rect.x + corner_width, rect.y + rect.height);
	cairo_curve_to(cairo,
		rect.x, rect.y + rect.height,
		rect.x, rect.y + rect.height,
		rect.x, rect.y + rect.height - corner_height);
	cairo_line_to(cairo, rect.x, rect.y + corner_height);
	cairo_curve_to(cairo,
		rect.x, rect.y,
		rect.x, rect.y,
		rect.x + corner_width, rect.y);
	cairo_close_path(cairo);
}



void Widget::use_rect(const Rect& rect)
{
	cairo_rectangle(gui->cairo(), rect.x, rect.y, rect.width, rect.height);
}

void Widget::use_color(const Color& color)
{
	cairo_set_source_rgba(gui->cairo(), color.red, color.green, color.blue, color.alpha);
}


void Widget::use_font(CairoFontconfigFont* fc_font)
{
	auto cairo = gui->cairo();
	cairo_set_font_face(cairo, fc_font->font_face);
	if (fc_font->size != 0)
		cairo_set_font_size(cairo, fc_font->size);
}



