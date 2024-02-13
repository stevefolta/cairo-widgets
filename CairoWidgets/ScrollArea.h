#pragma once

#include "Widget.h"
#include "CairoGUI.h"

class Scrollbar;


class ScrollArea : public Widget {
	public:
		ScrollArea(CairoGUI* gui_in, Widget* contents_in = nullptr, Rect rect_in = {});
		~ScrollArea();

		Widget* contents = nullptr;
		// Give this to the contents:
		CairoGUI* cairo_gui() { return &adjusted_cairo_gui; }

		void paint();
		void mouse_pressed(int x, int y);
		bool mouse_released(int x, int y);
		void mouse_moved(int x, int y);
		bool sticky_tracking();
		void scroll_down(int x, int y);
		void scroll_up(int x, int y);

		// For elements that aren't clipped to the area, like popped-up popup
		// menus:
		virtual void paint_foreground() {}

		struct Style {
			double scrollbar_width = 6.0;
			double scrollbar_inset = 10.0;
			Color border_color = { 0.0, 0.0, 0.0 };
			double border_width = 1.0;
			double relative_wheel_scroll_amount = 0.1;
			bool always_show_scrollbar = false;
			};
		static Style default_style;
		Style style = default_style;

	protected:
		class ScrolledCairoGUI : public CairoGUI {
			public:
				ScrolledCairoGUI(ScrollArea* scroll_area_in)
					: scroll_area(scroll_area_in) {}
				ScrollArea* scroll_area;

				cairo_t* cairo() { return scroll_area->gui->cairo(); }
				void refresh() { scroll_area->gui->refresh(); }
				const char* default_font() { return scroll_area->gui->default_font(); }
				Rect popup_limits();
			};
		friend class ScrolledCairoGUI;
		ScrolledCairoGUI adjusted_cairo_gui;

		Scrollbar* scrollbar;
		Widget* tracking_widget = nullptr;

		void update_scrollbar();
		Rect scrollbar_rect();
	};

