#pragma once

#include "Widget.h"
#include <vector>


class CompoundWidget : public Widget {
	public:
		CompoundWidget(CairoGUI* gui_in, Rect rect_in)
			: Widget(gui_in, rect_in) {}
		~CompoundWidget();

		void paint();
		bool contains(double x, double y);
		void mouse_pressed(int x, int y);
		bool mouse_released(int x, int y);
		void mouse_moved(int x, int y);
		bool sticky_tracking();
		int preferred_cursor(int x, int y);

		virtual void layout() {}

	protected:
		std::vector<Widget*> all_widgets;
		Widget* tracking_widget = nullptr;

		virtual void widget_accepted(Widget* widget) {}

		void remove_widget(Widget* widget);
		void delete_widget(Widget* widget);
	};

