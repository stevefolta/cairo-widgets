#pragma once

#include "Widget.h"
#include "TimeSeconds.h"
#include <cairo/cairo.h>
#include <string>


class StringInputBox : public Widget {
	public:
		StringInputBox(CairoGUI* gui_in, Rect rect_in = {});

		std::string value;
		bool has_focus = true;
		std::string label;
		double force_label_width = 0.0;

		void paint();
		int next_update_ms();

		bool key_pressed(int c);
		bool special_key_pressed(SpecialKey key);

		void mouse_pressed(int x, int y);
		bool mouse_released(int x, int y);
		void mouse_moved(int x, int y);
		int preferred_cursor(int x, int y) { return TextCursor; }

		void focus() { has_focus = true; }
		void defocus() { has_focus = false; }
		bool accepts_input() { return true; }

		void select_start();
		void select_end();
		void select_all();
		bool is_at_end();

		double drawn_label_width();

		struct Style {
			Color color = { 0.0, 0.0, 0.0 }, border_color = { 0.0, 0.0, 0.0 }, cursor_color = { 0.0, 0.0, 0.0 };
			Color selection_color = { 0.0, 0.5, 1.0, 0.3 };
			const char* font = nullptr;
			cairo_font_weight_t font_weight = CAIRO_FONT_WEIGHT_NORMAL;
			double relative_font_size = 0.75;
			double border_width = 1.0;
			double cursor_width = 1.0;
			};
		Style style = default_style;
		CairoFontconfigFont* fc_font = nullptr;
		static Style default_style;
		static double cursor_flash_rate, double_click_time;

	protected:
		TimeSeconds tick_phase_time, tick_update_time, last_click_time;
		int selection_start = 0, selection_end = 0;
		int drag_start = -1, drag_start_end = -1;
		int clicks = 0;

		struct DrawLocs { double margin, baseline; };
		DrawLocs setup_font();
		int char_pos_for_mouse_pos(int x);
		void extend_selection_to_word_start();
		void extend_selection_to_word_end();
		static std::string utf8_for_char(unsigned int c);
		static bool is_word_char(char c);
	};

