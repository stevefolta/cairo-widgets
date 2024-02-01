#pragma once

#include "Widget.h"
#include <cairo/cairo.h>


class Button : public Widget {
	public:
		Button(
			CairoGUI* gui,
			Rect rect,
			const char* label_in)
			: Widget(gui, rect), label(label_in)
		{}
		Button(CairoGUI* gui, const char* label_in, Rect rect = {})
			: Widget(gui, rect), label(label_in) {}

		bool enabled = true;

		void paint();
		void mouse_pressed(int x, int y);
		bool mouse_released(int x, int y);
		void mouse_moved(int x, int y);

		struct Style {
			const char* font = nullptr;
			cairo_font_weight_t font_weight = CAIRO_FONT_WEIGHT_BOLD;
			double relative_label_size = 0.6;
			double corner_size = 8.0;
			double border_width = 2.0;
			};
		Style style = default_style;
		static Style default_style;

		// For backwards compatibility.  Now, it's more idiomatic to set members of
		// "style" directly.
		void set_font(const char* font_in, cairo_font_weight_t weight_in) {
			style.font = font_in;
			style.font_weight = weight_in;
			}

	protected:
		const char* label;
		bool pressed = false, is_mouse_over = false;
	};

