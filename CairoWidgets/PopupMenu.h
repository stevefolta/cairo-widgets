#pragma once

#include "Widget.h"
#include <vector>
#include <string>
#include <cairo/cairo.h>


class PopupMenu : public Widget {
	public:
		PopupMenu(CairoGUI* gui_in, const std::vector<std::string>& items_in = {}, Rect rect_in = {});

		void paint();
		void mouse_pressed(int x, int y);
		bool mouse_released(int x, int y);
		void mouse_moved(int x, int y);

		virtual bool item_is_active(int which_item) { return true; }
		virtual bool item_is_checked(int which_item) { return false; }

		double natural_width(double for_height = 0);
		double natural_label_width(double for_height = 0);

		std::string label;
		double force_label_width = 0.0;
		int selected_item = 0;
		bool has_checked_items = false;

		struct Style {
			double margin = 6.0;
			Color border_color = { 0.0, 0.0, 0.0 };
			Color background_color = { 1.0, 1.0, 1.0 };
			Color selected_background_color = { 0.75, 0.75, 0.75 };
			Color hovering_overlay_color = { 0, 0.5, 1, 0.1 };
			Color foreground_color = { 0.0, 0.0, 0.0 };
			Color selected_foreground_color = { 0.0, 0.0, 0.0 };
			Color inactive_foreground_color = { 0.5, 0.5, 0.5 };
			Color arrow_color = { 0.3, 0.3, 0.3 };
			const char* font = nullptr;
			cairo_font_weight_t font_weight = CAIRO_FONT_WEIGHT_NORMAL;
			double border_width = 1.0;
			double relative_text_size = 0.75;
			double relative_arrow_size = 0.5;
			double relative_roundedness = 0.1;
			const char* checkmark_string = nullptr;
			};
		static Style default_style;
		Style style = default_style;

		// "max_bottom", if greater than zero, will constrain the bottom of the
		// popped-up menu.
		double max_bottom = 0.0;

	protected:
		std::vector<std::string> items;
		bool is_up = false;
		bool hovering = false;
		int initial_selected_item = -1;

		virtual void draw_item(int which_item, Rect item_rect, bool selected);
		Rect up_rect();
	};

