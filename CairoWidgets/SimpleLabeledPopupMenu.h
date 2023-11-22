#pragma once

#include "Widget.h"
#include <string>
#include <vector>

class SimplePopupMenu;
class SimplePopupMenuLabel;


class SimpleLabeledPopupMenu : public Widget {
	public:
		SimpleLabeledPopupMenu(
			CairoGUI* gui_in,
			std::string label,
			const std::vector<std::string>& items = {},
			Rect rect_in = {});
		~SimpleLabeledPopupMenu();

		void paint();
		void mouse_pressed(int x, int y);
		bool mouse_released(int x, int y);
		void mouse_moved(int x, int y);

		// "label_width" can be given, which is useful if you have popups stacked
		// vertically and want them all to line up.  Otherwise, the menu will be
		// right next to the label.
		void layout(double label_width = 0.0);

		int selected_item();
		double label_width();

		SimplePopupMenu* menu;
		SimplePopupMenuLabel* label;

	protected:
		bool tracking_menu = false;
	};

