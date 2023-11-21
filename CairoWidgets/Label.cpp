#include "Label.h"
#include "CairoGUI.h"

Color Label::default_color = { 0.0, 0.0, 0.0 };


void Label::paint()
{
	auto cairo = gui->cairo();
	cairo_save(cairo);

#ifdef DEBUG_LABEL_BOUNDARIES
	cairo_rectangle(cairo, rect.x, rect.y, rect.width, rect.height);
	cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);
	cairo_set_line_width(cairo, 1.0);
	cairo_stroke(cairo);
#endif

	cairo_select_font_face(cairo, (font ? font : gui->default_font()), CAIRO_FONT_SLANT_NORMAL, font_weight);
	cairo_set_font_size(cairo, rect.height);
	cairo_font_extents_t font_extents;
	cairo_font_extents(cairo, &font_extents);
	double x = rect.x;
	if (justification != LeftJustified) {
		cairo_text_extents_t text_extents;
		cairo_text_extents(cairo, label.c_str(), &text_extents);
		if (justification == RightJustified)
			x += rect.width - (text_extents.width + text_extents.x_bearing);
		else
			x += (rect.width - text_extents.width) / 2 - text_extents.x_bearing;
		}
	cairo_move_to(cairo, x, rect.y + font_extents.ascent);
	cairo_set_source_rgb(cairo, color.red, color.green, color.blue);
	cairo_show_text(cairo, label.c_str());

	cairo_restore(cairo);
}


double Label::drawn_width()
{
	auto cairo = gui->cairo();
	cairo_save(cairo);

	cairo_select_font_face(cairo, (font ? font : gui->default_font()), CAIRO_FONT_SLANT_NORMAL, font_weight);
	cairo_set_font_size(cairo, rect.height);
	cairo_text_extents_t text_extents;
	cairo_text_extents(cairo, label.c_str(), &text_extents);

	cairo_restore(cairo);

	return text_extents.width + text_extents.x_bearing;
}



