#pragma once

#include <stdint.h>

class CairoGUI;
class Button;
class SimplePopupMenu;
class Widget;


class ExampleWindow {
	public:
		ExampleWindow(CairoGUI* cairo_gui_in);
		~ExampleWindow();

		void paint();
		void resize(double new_width, double new_height);
		void mouse_pressed(int32_t x, int32_t y, int button);
		void mouse_released(int32_t x, int32_t y, int button);
		void mouse_moved(int32_t x, int32_t y);

	protected:
		CairoGUI* cairo_gui;
		Button* button = nullptr;
		SimplePopupMenu* menu = nullptr;
		SimplePopupMenu* color_menu = nullptr;
		Widget* tracking_widget = nullptr;
		double width, height;

		void layout();
	};

