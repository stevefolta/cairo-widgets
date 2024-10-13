#pragma once

#include "Rect.h"

class CairoGUI;
class CairoFontconfigFont;


struct Color {
	double red, green, blue, alpha = 1.0;
	};

enum SpecialKey {
	UpArrow, DownArrow, LeftArrow, RightArrow,
	PageUp, PageDown, HomeKey, EndKey,
	};


class Widget {
	public:
		enum {
			PointerCursor, TextCursor,
			};

		Widget(CairoGUI* gui_in, Rect rect_in)
			: rect(rect_in), gui(gui_in) {}
		virtual ~Widget() {}

		Rect rect;
		virtual bool contains(double x, double y);

		virtual void paint() {}
		virtual void mouse_pressed(int x, int y) {}
		virtual bool mouse_released(int x, int y) { return false; }
			// true => widget has been "accepted" (eg. a button was clicked and not canceled)
		virtual void mouse_moved(int x, int y) {}
		virtual bool sticky_tracking() { return false; }
			// true => the widget is still "tracking" after mouse_released()
			// return false.  Eg., a PopupMenu that is still popped-up after being
			// tapped.
		virtual void scroll_down(int x, int y) {}
		virtual void scroll_up(int x, int y) {}
		virtual bool key_pressed(int c) { return false; }
			// true => handled.
		virtual bool special_key_pressed(SpecialKey key) { return false; }
			// true => handled.
		virtual int preferred_cursor(int x, int y) { return PointerCursor; }
		virtual int next_update_ms() { return -1; } 	// -1 => not needed.

		// Key focus.
		virtual void focus() {}
		virtual void defocus() {}
		virtual bool accepts_input() { return false; }

		void move_right_to(double right) {
			rect.x = right - rect.width;
			}
		void move_bottom_to(double bottom) {
			rect.y = bottom - rect.height;
			}

	protected:
		CairoGUI* gui;

		// Utilities.
		void rounded_rect(Rect rect, double corner_size);
		void rounded_rect(Rect rect, double corner_width, double corner_height);
		void use_rect(const Rect& rect);
		void use_color(const Color& color);
		void use_font(CairoFontconfigFont* fc_font);
	};

