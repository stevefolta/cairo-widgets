#include "ExampleWindowWidget.h"
#include "Button.h"
#include "PopupMenu.h"
#include "StringInputBox.h"
#include "Checkbox.h"
#include "CairoGUI.h"


static const double margin = 10.0;
static const double default_button_width = 80.0;
static const double default_button_height = 24.0;
static const double default_menu_width = 120.0;
static const double default_menu_height = 24.0;
static const double default_labeled_menu_h_spacing = 30.0;
static const double default_string_input_box_width = 500.0;
static const double default_string_input_box_height = 28.0;
static const double default_checkbox_height = 20.0;
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


ExampleWindowWidget::ExampleWindowWidget(CairoGUI* gui_in)
	: CompoundWidget(gui_in)
{
	button = new Button(gui, "OK");
	menu = new PopupMenu(gui, { "Yes", "No", "Maybe" });
	color_menu = new CheckedPopupMenu(gui, { "Colors", "Red", "Green", "Blue" });
	checkbox = new Checkbox(gui, "Checkbox");
	low_menu = new PopupMenu(gui, { "Low", "Lower", "Lowest" });
	low_menu->label = "How low: ";
	std::vector<std::string> menu_items = { "Alpha", "Beta", "Gamma", "Interrobang" };
	std::vector<std::string> menu_names = { "One", "2", "Threeee" };
	for (int i = 0; i < 3; ++i) {
		auto popup = new PopupMenu(gui, menu_items);
		popup->label = "Unaligned " + menu_names[i] + ": ";
		unaligned_popups.push_back(popup);
		popup = new PopupMenu(gui, menu_items);
		popup->label = "Aligned " + menu_names[i] + ": ";
		aligned_popups.push_back(popup);
		}
	string_input_box = new StringInputBox(gui);
	string_input_box->value = "Hamburgefons";
	string_input_box->label = "Input: ";
	string_input_box->select_all();

	all_widgets = {
		button, menu, color_menu, checkbox, low_menu, string_input_box,
		};
	for (auto menu: unaligned_popups)
		all_widgets.push_back(menu);
	for (auto menu: aligned_popups)
		all_widgets.push_back(menu);
	string_input_box->focus();
	focused_widget = string_input_box;
}


int ExampleWindowWidget::next_update_ms()
{
	return focused_widget ? focused_widget->next_update_ms() : -1;
}


void ExampleWindowWidget::layout()
{
	auto spacing = default_spacing;
	auto button_width = default_button_width;
	auto button_height = default_button_height;
	auto menu_width = default_menu_width;
	auto menu_height = default_menu_height;
	auto labeled_menu_h_spacing = default_labeled_menu_h_spacing;
	auto string_input_box_width = default_string_input_box_width;
	auto string_input_box_height = default_string_input_box_height;
	auto checkbox_height = default_checkbox_height;
	if (rect.height > 1000) {
		spacing *= 2;
		button_width *= 2;
		button_height *= 2;
		menu_width *= 2;
		menu_height *= 2;
		labeled_menu_h_spacing *= 2;
		string_input_box_width *= 2;
		string_input_box_height *= 2;
		checkbox_height *= 2;
		menu->style.margin = 12.0;
		color_menu->style.margin = 12.0;
		}

	menu->rect = { margin, margin, menu_width, menu_height };
	auto top = margin + menu_height + spacing;
	button->rect = { margin, top, button_width, button_height };
	top += button_height + spacing;
	color_menu->rect = { margin, top, menu_width, menu_height };
	top += menu_height + spacing;
	checkbox->rect = { margin, top, checkbox->drawn_width(checkbox_height), checkbox_height };
	top += checkbox_height + spacing;

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

	auto low_top = rect.height - margin - menu_height;
	low_menu->rect = { margin, low_top, menu_width, menu_height };
	low_menu->rect.width = low_menu->natural_width();
}


void ExampleWindowWidget::key_pressed(int c)
{
	if (focused_widget)
		focused_widget->key_pressed(c);
}

void ExampleWindowWidget::special_key_pressed(SpecialKey key)
{
	if (focused_widget)
		focused_widget->special_key_pressed(key);
}


void ExampleWindowWidget::widget_accepted(Widget* widget)
{
	if (widget == color_menu) {
		color_menu->toggle_selected_item();
		color_menu->selected_item = 0;
		}
}


