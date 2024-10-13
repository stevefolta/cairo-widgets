#pragma once

#include "CompoundWidget.h"

class Button;
class PopupMenu;
class CheckedPopupMenu;
class StringInputBox;
class Checkbox;


class ExampleWindowWidget : public CompoundWidget {
	public:
		ExampleWindowWidget(CairoGUI* gui_in);

		int next_update_ms();

		void layout();

		bool key_pressed(int c);
		bool special_key_pressed(SpecialKey key);

	protected:
		Button* button = nullptr;
		PopupMenu* menu = nullptr;
		CheckedPopupMenu* color_menu = nullptr;
		Checkbox* checkbox = nullptr;
		PopupMenu* low_menu = nullptr;
		std::vector<PopupMenu*> unaligned_popups;
		std::vector<PopupMenu*> aligned_popups;
		StringInputBox* string_input_box = nullptr;

		Widget* focused_widget = nullptr;

		void widget_accepted(Widget* widget);
	};

