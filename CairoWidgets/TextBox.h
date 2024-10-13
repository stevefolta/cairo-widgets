#pragma once

#include "Widget.h"
#include <string>
#include <cairo/cairo.h>

class CairoFontconfigFont;


class TextBox : public Widget {
	public:
		TextBox(CairoGUI* gui_in, std::string text_in = "", Rect rect_in = {})
			: Widget(gui_in, rect), text(text_in) {}

		std::string text;

		struct Style {
			Color color = { 0.0, 0.0, 0.0 };
			const char* font = nullptr;
			double font_size = 12.0;
			cairo_font_weight_t font_weight = CAIRO_FONT_WEIGHT_NORMAL;
			};
		Style style = default_style;
		CairoFontconfigFont* fc_font = nullptr;
		static Style default_style;

		void paint();

	protected:
		std::string_view rtrim(const std::string_view& str);
	};

