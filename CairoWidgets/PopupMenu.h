#pragma once

#include "Widget.h"
#include <vector>
#include <string>
#include <cairo/cairo.h>


class PopupMenu : public Widget {
	public:
		PopupMenu(CairoGUI* gui_in, const std::vector<std::string>& items_in = {}, Rect rect_in = {});

		void paint();
		bool contains(double x, double y);
		void mouse_pressed(int x, int y);
		bool mouse_released(int x, int y);
		void mouse_moved(int x, int y);
		bool sticky_tracking();

		virtual bool item_is_active(int which_item) { return true; }
		virtual bool item_is_checked(int which_item) { return false; }

		double natural_width(double for_height = 0);
		double natural_label_width(double for_height = 0);

		std::string label;
		double force_label_width = 0.0;
		std::vector<std::string> items;
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
			double tap_slop = 3.0;
			};
		static Style default_style;
		Style style = default_style;

	protected:
		bool is_up = false, is_tapped = false;
		bool hovering = false;
		int initial_selected_item = -1;
		double tap_x = -1, tap_y = -1;

		virtual void draw_item(int which_item, Rect item_rect, bool selected);
		Rect up_rect();
	};

