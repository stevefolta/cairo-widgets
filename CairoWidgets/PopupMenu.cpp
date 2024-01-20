#include "PopupMenu.h"
#include "CairoGUI.h"


PopupMenu::PopupMenu(CairoGUI* gui_in, const std::vector<std::string>& items_in, Rect rect_in)
	: Widget(gui_in, rect), items(items_in)
{
}


void PopupMenu::paint()
{
	auto cairo = gui->cairo();
	if (cairo == nullptr)
		return;
	cairo_save(cairo);

	// Label.
	double label_width = (force_label_width > 0.0 ? force_label_width : natural_label_width());
	if (!label.empty()) {
		cairo_select_font_face(
			cairo,
			(font ? font : gui->default_font()),
			CAIRO_FONT_SLANT_NORMAL,
			font_weight);
		cairo_set_font_size(cairo, rect.height * relative_text_size);
		cairo_text_extents_t em_box;
		cairo_text_extents(cairo, "M", &em_box);
		double baseline = rect.y + (rect.height + em_box.height) / 2;
		cairo_move_to(cairo, rect.x, baseline);
		use_color(foreground_color);
		cairo_show_text(cairo, label.c_str());
		}

	if (is_up) {
		auto popped_rect = up_rect();

		// Items.
		Rect item_rect = popped_rect;
		item_rect.height = rect.height;
		int num_items = items.size();
		for (int which_item = 0; which_item < num_items; ++which_item) {
			bool selected = (which_item == selected_item && item_is_active(which_item));
			draw_item(which_item, item_rect, selected);
			item_rect.y += rect.height;
			}

		// Border.
		cairo_rectangle(cairo, popped_rect.x, popped_rect.y, popped_rect.width, popped_rect.height);
		cairo_set_source_rgb(cairo, border_color.red, border_color.green, border_color.blue);
		cairo_set_line_width(cairo, border_width);
		cairo_stroke(cairo);
		}

	else {
		Rect menu_rect = { rect.x + label_width, rect.y, rect.width - label_width, rect.height };
		cairo_save(cairo);
		rounded_rect(menu_rect, menu_rect.height * relative_roundedness);
		auto border_path = cairo_copy_path(cairo);
		cairo_clip_preserve(cairo);

		// Draw the background.
		use_color(background_color);
		cairo_fill(cairo);
		//*** TODO

		// Draw the current item.
		if (selected_item >= 0 && selected_item < (int) items.size())
			draw_item(selected_item, menu_rect, false);

		// Arrow.
		cairo_select_font_face(cairo, (font ? font : gui->default_font()), CAIRO_FONT_SLANT_NORMAL, font_weight);
		cairo_set_font_size(cairo, menu_rect.height * relative_arrow_size);
		cairo_text_extents_t text_extents;
		cairo_text_extents(cairo, "M", &text_extents);
		double baseline = menu_rect.y + (menu_rect.height + text_extents.height) / 2;
		cairo_text_extents(cairo, "\u25BC", &text_extents);
		cairo_move_to(cairo, menu_rect.x + menu_rect.width - margin - text_extents.x_advance, baseline);
		use_color(arrow_color);
		cairo_show_text(cairo, "\u25BC");

		// Border.
		cairo_restore(cairo);
		cairo_append_path(cairo, border_path);
		use_color(border_color);
		cairo_set_line_width(cairo, border_width);
		cairo_stroke(cairo);

		cairo_path_destroy(border_path);
		}

	cairo_restore(cairo);
}


void PopupMenu::mouse_pressed(int x, int y)
{
	initial_selected_item = selected_item;
	is_up = true;
}

bool PopupMenu::mouse_released(int x, int y)
{
	if (!is_up)
		return false;

	bool accepted = up_rect().contains(x, y) && item_is_active(selected_item);
	if (!accepted)
		selected_item = initial_selected_item;

	is_up = false;
	return accepted;
}

void PopupMenu::mouse_moved(int x, int y)
{
	if (!is_up)
		return;

	Rect full_rect = up_rect();
	selected_item = (y - full_rect.y) / rect.height;
}


void PopupMenu::draw_item(int which_item, Rect item_rect, bool selected)
{
	auto cairo = gui->cairo();

	// Background.
	const auto& color = (selected ? selected_background_color : background_color);
	use_rect(item_rect);
	use_color(color);
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
	double baseline = item_rect.y + (item_rect.height + em_box.height) / 2;
	cairo_move_to(cairo, item_rect.x + margin, baseline);
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


Rect PopupMenu::up_rect()
{
	Rect up_rect = rect;
	auto label_width = (force_label_width ? force_label_width : natural_label_width());
	up_rect.x += label_width;
	up_rect.width -= label_width;
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


double PopupMenu::natural_width()
{
	auto cairo = gui->cairo();
	cairo_save(cairo);
	auto label_width = (force_label_width ? force_label_width : natural_label_width());

	// Set up the font.
	cairo_select_font_face(cairo, (font ? font : gui->default_font()), CAIRO_FONT_SLANT_NORMAL, font_weight);
	cairo_set_font_size(cairo, rect.height * relative_text_size);
	cairo_text_extents_t text_extents;

	// Find the maximum item width.
	double max_item_width = 0;
	for (auto& item: items) {
		cairo_text_extents(cairo, item.c_str(), &text_extents);
		if (text_extents.x_advance > max_item_width)
			max_item_width = text_extents.x_advance;
		}

	// Checkmark?
	double checkmark_width = 0;
	if (has_checked_items) {
		checkmark_string = (this->checkmark_string ? this->checkmark_string : "\u2713");
		cairo_text_extents(cairo, checkmark_string, &text_extents);
		checkmark_width = text_extents.x_advance;
		}

	// Arrow.
	cairo_text_extents(cairo, "\u25BC", &text_extents);
	double arrow_width = text_extents.x_advance;
	// This doesn't seem to need the margin added in...

	cairo_restore(cairo);
	return label_width + 2 * margin + max_item_width + checkmark_width + arrow_width;
}

double PopupMenu::natural_label_width()
{
	if (label.empty())
		return 0;

	auto cairo = gui->cairo();
	cairo_save(cairo);
	cairo_select_font_face(
		cairo,
		(font ? font : gui->default_font()),
		CAIRO_FONT_SLANT_NORMAL,
		font_weight);
	cairo_set_font_size(cairo, rect.height * relative_text_size);
	cairo_text_extents_t text_extents;
	cairo_text_extents(cairo, label.c_str(), &text_extents);
	cairo_restore(cairo);
	return text_extents.x_advance;
}



