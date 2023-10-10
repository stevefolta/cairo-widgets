#include "SimplePopupMenu.h"
#include "CairoGUI.h"


SimplePopupMenu::SimplePopupMenu(CairoGUI* gui_in, const std::vector<std::string>& items_in, Rect rect_in)
	: Widget(gui_in, rect), items(items_in)
{
}


void SimplePopupMenu::paint()
{
	auto cairo = gui->cairo();
	if (cairo == nullptr)
		return;
	cairo_save(cairo);

	if (is_up) {
		auto rect = up_rect();

		// Items.
		double item_top = rect.y;
		int num_items = items.size();
		for (int which_item = 0; which_item < num_items; ++which_item) {
			bool selected = (which_item == selected_item && item_is_active(which_item));
			draw_item(which_item, item_top, selected);
			item_top += this->rect.height;
			}

		// Border.
		cairo_rectangle(cairo, rect.x, rect.y, rect.width, rect.height);
		cairo_set_source_rgb(cairo, border_color.red, border_color.green, border_color.blue);
		cairo_set_line_width(cairo, border_width);
		cairo_stroke(cairo);
		}

	else {
		// Draw the current item.
		if (selected_item >= 0 && selected_item < (int) items.size())
			draw_item(selected_item, rect.y, false);

		// Border.
		cairo_rectangle(cairo, rect.x, rect.y, rect.width, rect.height);
		cairo_set_source_rgb(cairo, border_color.red, border_color.green, border_color.blue);
		cairo_set_line_width(cairo, border_width);
		cairo_stroke(cairo);

		// Arrow.
		cairo_select_font_face(cairo, (font ? font : gui->default_font()), CAIRO_FONT_SLANT_NORMAL, font_weight);
		cairo_set_font_size(cairo, rect.height * relative_arrow_size);
		cairo_text_extents_t text_extents;
		cairo_text_extents(cairo, "M", &text_extents);
		double baseline = rect.y + (rect.height + text_extents.height) / 2;
		cairo_text_extents(cairo, "\u25BC", &text_extents);
		cairo_move_to(cairo, rect.x + rect.width - margin - text_extents.x_advance, baseline);
		cairo_set_source_rgb(cairo, foreground_color.red, foreground_color.green, foreground_color.blue);
		cairo_show_text(cairo, "\u25BC");
		}

	cairo_restore(cairo);
}


void SimplePopupMenu::mouse_pressed(int x, int y)
{
	initial_selected_item = selected_item;
	is_up = true;
}

bool SimplePopupMenu::mouse_released(int x, int y)
{
	if (!is_up)
		return false;

	bool accepted = up_rect().contains(x, y) && item_is_active(selected_item);
	if (!accepted)
		selected_item = initial_selected_item;

	is_up = false;
	return accepted;
}

void SimplePopupMenu::mouse_moved(int x, int y)
{
	if (!is_up)
		return;

	Rect full_rect = up_rect();
	selected_item = (y - full_rect.y) / rect.height;
}


void SimplePopupMenu::draw_item(int which_item, double top, bool selected)
{
	auto cairo = gui->cairo();

	// Background.
	const auto& color = (selected ? selected_background_color : background_color);
	cairo_rectangle(cairo, rect.x, top, rect.width, rect.height);
	cairo_set_source_rgb(cairo, color.red, color.green, color.blue);
	cairo_fill(cairo);

	// Set up the font.
	cairo_select_font_face(cairo, (font ? font : gui->default_font()), CAIRO_FONT_SLANT_NORMAL, font_weight);
	cairo_set_font_size(cairo, rect.height * relative_text_size);
	cairo_text_extents_t em_box;
	cairo_text_extents(cairo, "M", &em_box);

	// Set up the checkmark.
	const char* checkmark_string = nullptr;
	double checkmark_width = 0;
	if (has_checked_items) {
		checkmark_string = (this->checkmark_string ? this->checkmark_string : "\u2713");
		cairo_text_extents_t checkmark_extents;
		cairo_text_extents(cairo, checkmark_string, &checkmark_extents);
		checkmark_width = checkmark_extents.x_advance;
		}

	// Draw the item.
	double baseline = top + (rect.height + em_box.height) / 2;
	cairo_move_to(cairo, rect.x + margin, baseline);
	const auto& text_color =
		(selected ? selected_foreground_color :
		 item_is_active(which_item) ? foreground_color : inactive_foreground_color);
	cairo_set_source_rgb(cairo, text_color.red, text_color.green, text_color.blue);
	if (has_checked_items) {
		if (item_is_checked(which_item))
			cairo_show_text(cairo, checkmark_string);
		else
			cairo_rel_move_to(cairo, checkmark_width, 0);
		}
	cairo_show_text(cairo, items[which_item].c_str());
}


Rect SimplePopupMenu::up_rect()
{
	Rect up_rect = rect;
	if (initial_selected_item > 0) {
		up_rect.y -= initial_selected_item * rect.height;
		if (up_rect.y < 0)
			up_rect.y = 0;
		}
	up_rect.height = items.size() * rect.height;
	if (up_rect.y + up_rect.height > max_bottom && up_rect.height < max_bottom)
		up_rect.y = max_bottom - up_rect.height;
	return up_rect;
}



