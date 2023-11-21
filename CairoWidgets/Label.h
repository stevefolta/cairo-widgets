#pragma once

#include "Widget.h"
#include <cairo/cairo.h>
#include <string>


class Label : public Widget {
	public:
		enum {
			LeftJustified, Centered, RightJustified,
			};

		Label(CairoGUI* gui, std::string label_in, Rect rect = {})
			: Widget(gui, rect), label(label_in) {}

		std::string label;

		static Color default_color;
		Color color = default_color;
		const char* font = nullptr;
		cairo_font_weight_t font_weight = CAIRO_FONT_WEIGHT_BOLD;
		int justification = LeftJustified;

		void paint();

		double drawn_width();
	};

