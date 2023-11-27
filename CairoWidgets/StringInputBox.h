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

		void paint();
		int next_update_ms();

		void key_pressed(int c);
		void special_key_pressed(SpecialKey key);

		void mouse_pressed(int x, int y);
		bool mouse_released(int x, int y);
		void mouse_moved(int x, int y);

		void select_start();
		void select_end();
		void select_all();

		Color color = default_color, border_color = { 0.0, 0.0, 0.0 }, cursor_color = { 0.0, 0.0, 0.0 }, selection_color = default_selection_color;
		const char* font = nullptr;
		cairo_font_weight_t font_weight = CAIRO_FONT_WEIGHT_NORMAL;
		double relative_font_size = default_relative_font_size;
		double border_width = default_border_width;
		static Color default_color, default_selection_color;
		static double default_relative_font_size, default_border_width, cursor_flash_rate, cursor_width;
		static double double_click_time;

	protected:
		TimeSeconds start_time, last_click_time;
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

