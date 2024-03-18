#include "PopupMenu.h"
#include "CairoGUI.h"
#include <math.h>

PopupMenu::Style PopupMenu::default_style;


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
			(style.font ? style.font : gui->default_font()),
			CAIRO_FONT_SLANT_NORMAL,
			style.font_weight);
		cairo_set_font_size(cairo, rect.height * style.relative_text_size);
		cairo_text_extents_t em_box;
		cairo_text_extents(cairo, "M", &em_box);
		double baseline = rect.y + (rect.height + em_box.height) / 2;
		cairo_move_to(cairo, rect.x, baseline);
		use_color(style.foreground_color);
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
		use_color(style.border_color);
		cairo_set_line_width(cairo, style.border_width);
		cairo_stroke(cairo);
		}

	else {
		Rect menu_rect = { rect.x + label_width, rect.y, rect.width - label_width, rect.height };
		cairo_save(cairo);
		rounded_rect(menu_rect, menu_rect.height * style.relative_roundedness);
		auto border_path = cairo_copy_path(cairo);
		cairo_clip_preserve(cairo);

		// Draw the current item.
		if (selected_item >= 0 && selected_item < (int) items.size())
			draw_item(selected_item, menu_rect, false);

		// Arrow.
		cairo_select_font_face(cairo, (style.font ? style.font : gui->default_font()), CAIRO_FONT_SLANT_NORMAL, style.font_weight);
		cairo_set_font_size(cairo, menu_rect.height * style.relative_arrow_size);
		cairo_text_extents_t text_extents;
		cairo_text_extents(cairo, "M", &text_extents);
		double baseline = menu_rect.y + (menu_rect.height + text_extents.height) / 2;
		cairo_text_extents(cairo, "\u25BC", &text_extents);
		cairo_move_to(cairo, menu_rect.x + menu_rect.width - style.margin - text_extents.x_advance, baseline);
		use_color(style.arrow_color);
		cairo_show_text(cairo, "\u25BC");

		// Hovering overlay.
		if (hovering) {
			use_rect(menu_rect);
			use_color(style.hovering_overlay_color);
			cairo_fill(cairo);
			}

		cairo_restore(cairo);

		// Border.
		cairo_append_path(cairo, border_path);
		use_color(style.border_color);
		cairo_set_line_width(cairo, style.border_width);
		cairo_stroke(cairo);

		cairo_path_destroy(border_path);
		}

	cairo_restore(cairo);
}


bool PopupMenu::contains(double x, double y)
{
	return (is_up ? up_rect().contains(x, y) : rect.contains(x, y));
}

void PopupMenu::mouse_pressed(int x, int y)
{
	if (!is_up)
		initial_selected_item = selected_item;
	is_tapped = !is_up;
	is_up = true;
	tap_x = x;
	tap_y = y;
}

bool PopupMenu::mouse_released(int x, int y)
{
	if (!is_up)
		return false;

	// Check for "tap to open".
	mouse_moved(x, y);
	if (is_tapped) {
		// Stay up.
		return false;
		}

	bool accepted = up_rect().contains(x, y) && item_is_active(selected_item);
	if (!accepted)
		selected_item = initial_selected_item;

	is_up = false;
	mouse_moved(x, y);
	return accepted;
}

void PopupMenu::mouse_moved(int x, int y)
{
	if (!is_up) {
		double label_width = (force_label_width > 0.0 ? force_label_width : natural_label_width());
		Rect menu_rect = { rect.x + label_width, rect.y, rect.width - label_width, rect.height };
		hovering = menu_rect.contains(x, y);
		return;
		}

	if (is_tapped) {
		double tap_distance = fabs(x - tap_x);
		if (tap_distance > style.tap_slop)
			is_tapped = false;
		else {
			tap_distance = fabs(y - tap_y);
			if (tap_distance > style.tap_slop)
				is_tapped = false;
			}
		}

	Rect full_rect = up_rect();
	selected_item = (y - full_rect.y) / rect.height;
}

bool PopupMenu::sticky_tracking()
{
	return is_up;
}


void PopupMenu::draw_item(int which_item, Rect item_rect, bool selected)
{
	auto cairo = gui->cairo();

	// Background.
	const auto& color = (selected ? style.selected_background_color : style.background_color);
	use_rect(item_rect);
	use_color(color);
	cairo_fill(cairo);

	// Set up the font.
	cairo_select_font_face(cairo, (style.font ? style.font : gui->default_font()), CAIRO_FONT_SLANT_NORMAL, style.font_weight);
	cairo_set_font_size(cairo, rect.height * style.relative_text_size);
	cairo_text_extents_t em_box;
	cairo_text_extents(cairo, "M", &em_box);

	// Set up the checkmark.
	const char* checkmark_string = nullptr;
	double checkmark_width = 0;
	if (has_checked_items) {
		checkmark_string = (style.checkmark_string ? style.checkmark_string : "\u2713");
		cairo_text_extents_t checkmark_extents;
		cairo_text_extents(cairo, checkmark_string, &checkmark_extents);
		checkmark_width = checkmark_extents.x_advance;
		}

	// Draw the item.
	double baseline = item_rect.y + (item_rect.height + em_box.height) / 2;
	cairo_move_to(cairo, item_rect.x + style.margin, baseline);
	const auto& text_color =
		(selected ? style.selected_foreground_color :
		 item_is_active(which_item) ? style.foreground_color : style.inactive_foreground_color);
	use_color(text_color);
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
	auto popup_limits = gui->popup_limits();
	if (initial_selected_item > 0) {
		up_rect.y -= initial_selected_item * rect.height;
		if (up_rect.y < popup_limits.y)
			up_rect.y = popup_limits.y;
		}
	up_rect.height = items.size() * rect.height;
	if (popup_limits.height > 0) {
		auto max_bottom = popup_limits.y + popup_limits.height;
		if (up_rect.y + up_rect.height > max_bottom && up_rect.height < max_bottom)
			up_rect.y = max_bottom - up_rect.height;
		}
	return up_rect;
}


double PopupMenu::natural_width(double for_height)
{
	// Make sure we have a "cairo".  We'll make one if needed.
	auto cairo = gui->cairo();
	cairo_surface_t* created_surface = nullptr;
	if (cairo == nullptr) {
		created_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
		if (created_surface == nullptr)
			return 0;
		cairo = cairo_create(created_surface);
		}

	cairo_save(cairo);
	if (for_height <= 0)
		for_height = rect.height;
	auto label_width = (force_label_width ? force_label_width : natural_label_width(for_height));

	// Set up the font.
	cairo_select_font_face(cairo, (style.font ? style.font : gui->default_font()), CAIRO_FONT_SLANT_NORMAL, style.font_weight);
	cairo_set_font_size(cairo, for_height * style.relative_text_size);
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
		auto checkmark_string = (style.checkmark_string ? style.checkmark_string : "\u2713");
		cairo_text_extents(cairo, checkmark_string, &text_extents);
		checkmark_width = text_extents.x_advance;
		}

	// Arrow.
	cairo_text_extents(cairo, "\u25BC", &text_extents);
	double arrow_width = text_extents.x_advance;
	// This doesn't seem to need the margin added in...

	cairo_restore(cairo);
	if (created_surface) {
		cairo_destroy(cairo);
		cairo_surface_destroy(created_surface);
		}

	return label_width + 2 * style.margin + max_item_width + checkmark_width + arrow_width;
}

double PopupMenu::natural_label_width(double for_height)
{
	if (label.empty())
		return 0;

	// Make sure we have a "cairo".  We'll make one if needed.
	auto cairo = gui->cairo();
	cairo_surface_t* created_surface = nullptr;
	if (cairo == nullptr) {
		created_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
		if (created_surface == nullptr)
			return 0;
		cairo = cairo_create(created_surface);
		}

	if (for_height <= 0)
		for_height = rect.height;
	cairo_save(cairo);
	cairo_select_font_face(
		cairo,
		(style.font ? style.font : gui->default_font()),
		CAIRO_FONT_SLANT_NORMAL,
		style.font_weight);
	cairo_set_font_size(cairo, for_height * style.relative_text_size);
	cairo_text_extents_t text_extents;
	cairo_text_extents(cairo, label.c_str(), &text_extents);

	cairo_restore(cairo);
	if (created_surface) {
		cairo_destroy(cairo);
		cairo_surface_destroy(created_surface);
		}

	return text_extents.x_advance;
}



