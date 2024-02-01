#pragma once

#include <cairo/cairo.h>
#include "Rect.h"


class CairoGUI {
	public:
		virtual cairo_t* cairo() = 0;
		virtual void refresh() = 0;
		virtual const char* default_font() { return "Deja Vu Sans"; }

		// If width & height are positive, this represents the area that popup
		// elements (like a clicked PopupMenu) should try to stay within.
		virtual Rect popup_limits() { return { 0, 0, 0, 0 }; }
	};

