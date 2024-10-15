#pragma once

#include "Widget.h"
#include <vector>


class CompoundWidget : public Widget {
	public:
		CompoundWidget(CairoGUI* gui_in, Rect rect_in = {})
			: Widget(gui_in, rect_in) {}
		~CompoundWidget();

		void paint();
		bool contains(double x, double y);
		void mouse_pressed(int x, int y);
		bool mouse_released(int x, int y);
		void mouse_moved(int x, int y);
		bool sticky_tracking();
		int preferred_cursor(int x, int y);

		bool key_pressed(int c);
		bool special_key_pressed(SpecialKey key);
		void focus();
		void defocus();
		int next_update_ms() { return focused_widget ? focused_widget->next_update_ms() : -1; }

		virtual void layout() {}

		void focus_widget(Widget* new_widget);

	protected:
		std::vector<Widget*> all_widgets;
		Widget* tracking_widget = nullptr;
		Widget* focused_widget = nullptr;

		virtual void widget_accepted(Widget* widget) {}

		void remove_widget(Widget* widget);
		void delete_widget(Widget* widget);
	};

