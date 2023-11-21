#pragma once

#include "Widget.h"
#include <string>

class SimplePopupMenu;

// Like a Label, but meant to pair with a SimplePopupMenu.


class SimplePopupMenuLabel : public Widget {
	public:
		SimplePopupMenuLabel(CairoGUI* gui, std::string label_in, SimplePopupMenu* menu_in, Rect rect = {})
			: Widget(gui, rect), label(label_in), menu(menu_in) {}

		std::string label;

		void paint();

		double drawn_width();

	protected:
		SimplePopupMenu* menu;
	};


