#pragma once

#include <fontconfig/fontconfig.h>
#include <cairo/cairo-ft.h>
#include <string>


class CairoFontconfigFont {
	public:
		CairoFontconfigFont(const std::string& spec);
		~CairoFontconfigFont();

		cairo_font_face_t* font_face = nullptr;
		double size = 0;

	protected:
		FcPattern* pattern = nullptr;
	};

