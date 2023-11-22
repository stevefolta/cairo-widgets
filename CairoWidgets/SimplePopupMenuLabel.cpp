#include "SimplePopupMenuLabel.h"
#include "SimplePopupMenu.h"
#include "CairoGUI.h"


void SimplePopupMenuLabel::paint()
{
	auto cairo = gui->cairo();
	cairo_save(cairo);

	cairo_select_font_face(
		cairo,
		(menu->font ? menu->font : gui->default_font()),
		CAIRO_FONT_SLANT_NORMAL,
		menu->font_weight);
	cairo_set_font_size(cairo, menu->rect.height * menu->relative_text_size);
	cairo_text_extents_t em_box;
	cairo_text_extents(cairo, "M", &em_box);
	double baseline = rect.y + (rect.height + em_box.height) / 2;
	cairo_move_to(cairo, rect.x, baseline);
	use_color(menu->foreground_color);
	cairo_show_text(cairo, label.c_str());

	cairo_restore(cairo);
}


double SimplePopupMenuLabel::drawn_width()
{
	auto cairo = gui->cairo();
	cairo_save(cairo);

	cairo_select_font_face(
		cairo,
		(menu->font ? menu->font : gui->default_font()),
		CAIRO_FONT_SLANT_NORMAL,
		menu->font_weight);
	cairo_set_font_size(cairo, menu->rect.height * menu->relative_text_size);
	cairo_text_extents_t text_extents;
	cairo_text_extents(cairo, label.c_str(), &text_extents);

	cairo_restore(cairo);

	return text_extents.x_advance;
}


