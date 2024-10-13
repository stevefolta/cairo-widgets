#include "CairoFontconfigFont.h"


CairoFontconfigFont::CairoFontconfigFont(const std::string& spec)
{
	pattern = FcNameParse((const FcChar8*) spec.c_str());
	FcResult result = FcPatternGetDouble(pattern, FC_PIXEL_SIZE, 0, &size);
	if (result != FcResultMatch)
		result = FcPatternGetDouble(pattern, FC_SIZE, 0, &size);
	font_face = cairo_ft_font_face_create_for_pattern(pattern);
}


CairoFontconfigFont::~CairoFontconfigFont()
{
	cairo_font_face_destroy(font_face);
	FcPatternDestroy(pattern);
}


