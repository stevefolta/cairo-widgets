#include "ExampleWindow.h"
#include "Button.h"
#include "PopupMenu.h"
#include "StringInputBox.h"
#include "CairoGUI.h"
#include <iostream>

static const double margin = 10.0;
static const double default_button_width = 80.0;
static const double default_button_height = 24.0;
static const double default_menu_width = 120.0;
static const double default_menu_height = 24.0;
static const double default_labeled_menu_h_spacing = 30.0;
static const double default_string_input_box_width = 500.0;
static const double default_string_input_box_height = 28.0;
static const double default_spacing = 6.0;
static const Color background_color = { 1.0, 1.0, 1.0 };

class CheckedPopupMenu : public PopupMenu {
	public:
		CheckedPopupMenu(CairoGUI* gui_in, const std::vector<std::string>& items_in = {}, Rect rect_in = {})
			: PopupMenu(gui_in, items_in, rect_in), checked_items(items_in.size()) {
				has_checked_items = true;
				}
		bool item_is_active(int which_item) { return which_item > 0; }
		bool item_is_checked(int which_item) { return checked_items[which_item]; }
		void toggle_selected_item() {
			if (selected_item >= 0)
				checked_items[selected_item] = !checked_items[selected_item];
			}

	protected:
		std::vector<bool> checked_items;
	};


ExampleWindow::ExampleWindow(CairoGUI* cairo_gui_in)
	: cairo_gui(cairo_gui_in)
{
	button = new Button(cairo_gui, "OK");
	menu = new PopupMenu(cairo_gui, { "Yes", "No", "Maybe" });
	color_menu = new CheckedPopupMenu(cairo_gui, { "Colors", "Red", "Green", "Blue" });
	low_menu = new PopupMenu(cairo_gui, { "Low", "Lower", "Lowest" });
	low_menu->label = "How low: ";
	std::vector<std::string> menu_items = { "Alpha", "Beta", "Gamma", "Interrobang" };
	std::vector<std::string> menu_names = { "One", "2", "Threeee" };
	for (int i = 0; i < 3; ++i) {
		auto popup = new PopupMenu(cairo_gui, menu_items);
		popup->label = "Unaligned " + menu_names[i] + ": ";
		unaligned_popups.push_back(popup);
		popup = new PopupMenu(cairo_gui, menu_items);
		popup->label = "Aligned " + menu_names[i] + ": ";
		aligned_popups.push_back(popup);
		}
	string_input_box = new StringInputBox(cairo_gui);
	string_input_box->value = "Hamburgefons";
	string_input_box->label = "Input: ";
	string_input_box->select_all();
}


ExampleWindow::~ExampleWindow()
{
	for (auto menu: unaligned_popups)
		delete menu;
	for (auto menu: aligned_popups)
		delete menu;
	delete button;
	delete menu;
	delete color_menu;
	delete low_menu;
	delete string_input_box;
}


void ExampleWindow::paint()
{
	auto cairo = cairo_gui->cairo();
	cairo_push_group(cairo);

	// Draw the background.
	cairo_save(cairo);
	cairo_rectangle(cairo, 0, 0, width, height);
	cairo_set_source_rgb(cairo, background_color.red, background_color.green, background_color.blue);
	cairo_fill(cairo);
	cairo_restore(cairo);

	// Draw widgets, always drawing a popped-up menu on top.
	std::vector<Widget*> all_widgets = {
		button, menu, color_menu, low_menu, string_input_box,
		};
	for (auto menu: unaligned_popups)
		all_widgets.push_back(menu);
	for (auto menu: aligned_popups)
		all_widgets.push_back(menu);
	for (auto widget: all_widgets) {
		if (widget != tracking_widget)
			widget->paint();
		}
	if (tracking_widget)
		tracking_widget->paint();

	// Blit to screen.
	cairo_pop_group_to_source(cairo);
	cairo_paint(cairo);
}


void ExampleWindow::resize(double new_width, double new_height)
{
	width = new_width;
	height = new_height;
	layout();
}


void ExampleWindow::mouse_pressed(int32_t x, int32_t y, int button)
{
	if (button != 1)
		return;

	if (this->button->contains(x, y))
		tracking_widget = this->button;
	else if (menu->contains(x, y))
		tracking_widget = menu;
	else if (color_menu->contains(x, y))
		tracking_widget = color_menu;
	else if (string_input_box->contains(x, y))
		tracking_widget = string_input_box;
	else if (low_menu->contains(x, y))
		tracking_widget = low_menu;
	else {
		for (auto menu: unaligned_popups) {
			if (menu->contains(x, y)) {
				tracking_widget = menu;
				break;
				}
			}
		if (tracking_widget == nullptr) {
			for (auto menu: aligned_popups) {
				if (menu->contains(x, y)) {
					tracking_widget = menu;
					break;
					}
				}
			}
		}
	if (tracking_widget)
		tracking_widget->mouse_pressed(x, y);
}

void ExampleWindow::mouse_released(int32_t x, int32_t y, int button)
{
	if (button != 1)
		return;

	if (tracking_widget) {
		bool accepted = tracking_widget->mouse_released(x, y);
		if (accepted && tracking_widget == color_menu) {
			color_menu->toggle_selected_item();
			color_menu->selected_item = 0;
			}
		tracking_widget = nullptr;
		}
}

void ExampleWindow::mouse_moved(int32_t x, int32_t y)
{
	if (tracking_widget)
		tracking_widget->mouse_moved(x, y);
	else {
		// Enable hovering behavior for the menus.
		for (auto menu: { menu, (PopupMenu*) color_menu, low_menu })
			menu->mouse_moved(x, y);
		for (auto menu: unaligned_popups)
			menu->mouse_moved(x, y);
		for (auto menu: aligned_popups)
			menu->mouse_moved(x, y);
		}
}


void ExampleWindow::key_pressed(int c)
{
	string_input_box->key_pressed(c);
}

void ExampleWindow::special_key_pressed(SpecialKey key)
{
	string_input_box->special_key_pressed(key);
}


int ExampleWindow::next_update_ms()
{
	return string_input_box->next_update_ms();
}


void ExampleWindow::layout()
{
	auto spacing = default_spacing;
	auto button_width = default_button_width;
	auto button_height = default_button_height;
	auto menu_width = default_menu_width;
	auto menu_height = default_menu_height;
	auto labeled_menu_h_spacing = default_labeled_menu_h_spacing;
	auto string_input_box_width = default_string_input_box_width;
	auto string_input_box_height = default_string_input_box_height;
	if (height > 1000) {
		spacing *= 2;
		button_width *= 2;
		button_height *= 2;
		menu_width *= 2;
		menu_height *= 2;
		labeled_menu_h_spacing *= 2;
		string_input_box_width *= 2;
		string_input_box_height *= 2;
		menu->style.margin = 12.0;
		color_menu->style.margin = 12.0;
		}

	menu->rect = { margin, margin, menu_width, menu_height };
	auto top = margin + menu_height + spacing;
	button->rect = { margin, top, button_width, button_height };
	top += button_height + spacing;
	color_menu->rect = { margin, top, menu_width, menu_height };
	top += menu_height + spacing;

	auto menu_top = top;
	auto unaligned_menu_width = 0;
	for (auto menu: unaligned_popups) {
		auto width = menu->natural_width(menu_height);
		menu->rect = { margin, menu_top, width, menu_height };
		menu_top += menu_height + spacing;
		if (width > unaligned_menu_width)
			unaligned_menu_width = width;
		}
	double label_width = 0.0;
	for (auto menu: aligned_popups) {
		auto width = menu->natural_label_width(menu_height);
		if (width > label_width)
			label_width = width;
		}
	auto menu_left = margin + unaligned_menu_width + labeled_menu_h_spacing;
	for (auto menu: aligned_popups) {
		menu->force_label_width = label_width;
		menu->rect = { menu_left, top, menu->natural_width(menu_height), menu_height };
		top += menu_height + spacing;
		}

	string_input_box->rect = { margin, top, string_input_box_width, string_input_box_height };

	auto low_top = height - margin - menu_height;
	low_menu->rect = { margin, low_top, menu_width, menu_height };
	low_menu->rect.width = low_menu->natural_width();
	low_menu->max_bottom = height - margin;
}


