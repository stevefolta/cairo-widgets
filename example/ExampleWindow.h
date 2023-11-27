#pragma once

#include "Widget.h"
#include <vector>
#include <stdint.h>

class CairoGUI;
class Button;
class SimplePopupMenu;
class CheckedPopupMenu;
class SimplePopupMenuLabel;
class SimpleLabeledPopupMenu;
class StringInputBox;


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
		SimplePopupMenu* menu = nullptr;
		CheckedPopupMenu* color_menu = nullptr;
		SimplePopupMenu* low_menu = nullptr;
		SimplePopupMenuLabel* low_menu_label = nullptr;
		std::vector<SimpleLabeledPopupMenu*> unaligned_popups;
		std::vector<SimpleLabeledPopupMenu*> aligned_popups;
		StringInputBox* string_input_box = nullptr;
		Widget* tracking_widget = nullptr;
		double width, height;

		void layout();
	};

