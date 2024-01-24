#pragma once

#include "Widget.h"
#include <vector>
#include <stdint.h>

class CairoGUI;
class Button;
class PopupMenu;
class CheckedPopupMenu;
class StringInputBox;
class Checkbox;


class ExampleWindow {
	public:
		ExampleWindow(CairoGUI* cairo_gui_in);
		~ExampleWindow();

		void paint();
		void resize(double new_width, double new_height);
		void mouse_pressed(int32_t x, int32_t y, int button);
		void mouse_released(int32_t x, int32_t y, int button);
		void mouse_moved(int32_t x, int32_t y);
		void key_pressed(int c);
		void special_key_pressed(SpecialKey key);

		int next_update_ms();

	protected:
		CairoGUI* cairo_gui;
		Button* button = nullptr;
		PopupMenu* menu = nullptr;
		CheckedPopupMenu* color_menu = nullptr;
		Checkbox* checkbox = nullptr;
		PopupMenu* low_menu = nullptr;
		std::vector<PopupMenu*> unaligned_popups;
		std::vector<PopupMenu*> aligned_popups;
		StringInputBox* string_input_box = nullptr;
		Widget* tracking_widget = nullptr;
		double width, height;

		void layout();
	};

