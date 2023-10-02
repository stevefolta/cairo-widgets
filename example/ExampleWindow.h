#pragma once

class CairoGUI;
class Button;

class ExampleWindow {
	public:
		ExampleWindow(CairoGUI* cairo_gui_in);
		~ExampleWindow();

		void paint();
		void resize(double new_width, double new_height);

	protected:
		CairoGUI* cairo_gui;
		Button* button = nullptr;
		double width, height;

		void layout();
	};

