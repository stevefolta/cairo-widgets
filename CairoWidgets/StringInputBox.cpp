#include "StringInputBox.h"
#include "CairoGUI.h"
#include <time.h>
#include <math.h>
#include <stdint.h>


StringInputBox::Style StringInputBox::default_style;
double StringInputBox::cursor_flash_rate = 1.0;
double StringInputBox::double_click_time = 1.0;


StringInputBox::StringInputBox(CairoGUI* gui_in, Rect rect_in)
	: Widget(gui_in, rect_in)
{
	tick_phase_time = tick_update_time = TimeSeconds::now();
}


void StringInputBox::paint()
{
	auto cairo = gui->cairo();
	cairo_save(cairo);
	cairo_rectangle(cairo, rect.x, rect.y, rect.width, rect.height);
	cairo_clip(cairo);

	// Text setup.
	auto [ margin, baseline ] = setup_font();

	// Label.
	double cur_label_width = 0.0;
	cairo_text_extents_t text_extents;
	if (!label.empty() || force_label_width > 0) {
		cairo_move_to(cairo, rect.x, baseline);
		use_color(style.color);
		cairo_show_text(cairo, label.c_str());
		if (force_label_width > 0)
			cur_label_width = force_label_width;
		else {
			cairo_text_extents(cairo, label.c_str(), &text_extents);
			cur_label_width = text_extents.x_advance;
			}
		}

	// Selection.
	if (has_focus && selection_start != selection_end) {
		cairo_text_extents(cairo, value.substr(0, selection_start).c_str(), &text_extents);
		double left = rect.x + cur_label_width + margin + text_extents.x_advance;
		cairo_text_extents(cairo, value.substr(0, selection_end).c_str(), &text_extents);
		double right = rect.x + cur_label_width + margin + text_extents.x_advance;
		cairo_rectangle(cairo, left, rect.y + margin, right - left, rect.height - 2 * margin);
		use_color(style.selection_color);
		cairo_fill(cairo);
		}

	// Text.
	cairo_move_to(cairo, rect.x + cur_label_width + margin, baseline);
	use_color(style.color);
	cairo_show_text(cairo, value.c_str());

	// Cursor.
	if (has_focus && selection_start == selection_end) {
		// Flashing.
		bool show = fmod(tick_phase_time.elapsed_time(), cursor_flash_rate * 2) < cursor_flash_rate;
		if (show) {
			cairo_text_extents_t text_extents;
			cairo_text_extents(cairo, value.substr(0, selection_start).c_str(), &text_extents);
			double x = rect.x + cur_label_width + margin + text_extents.x_advance;
			cairo_move_to(cairo, x, rect.y + margin);
			cairo_line_to(cairo, x, rect.y + rect.height - margin);
			cairo_set_line_width(cairo, style.cursor_width);
			use_color(style.cursor_color);
			cairo_stroke(cairo);
			}
		tick_update_time = TimeSeconds::now();
		}

	// Border.
	// Don't clip it.
	cairo_restore(cairo);
	cairo_save(cairo);
	cairo_rectangle(cairo, rect.x + cur_label_width, rect.y, rect.width - cur_label_width, rect.height);
	cairo_set_line_width(cairo, style.border_width);
	use_color(style.border_color);
	cairo_stroke(cairo);

	cairo_restore(cairo);
}


int StringInputBox::next_update_ms()
{
	if (!has_focus || selection_start != selection_end)
		return -1;

	double elapsed_time = tick_update_time.elapsed_time();
	if (elapsed_time >= cursor_flash_rate)
		return 0;
	return (cursor_flash_rate - elapsed_time) * 1000;
}


void StringInputBox::key_pressed(int c)
{
	if (c == '\b') {
		// Backspace.
		if (selection_start == selection_end) {
			if (selection_start > 0) {
				int char_start = selection_start - 1;
				// Find the start of the UTF8 character.
				while (char_start > 0 && (value[char_start] & 0xC0) == 0x80)
					char_start -= 1;
				value = value.substr(0, char_start) + value.substr(selection_start);
				selection_start = selection_end = char_start;
				}
			}
		return;
		}
	else if (c == 0x7F) {
		// Delete.
		if (selection_start != selection_end) {
			value = value.substr(0, selection_start) + value.substr(selection_end);
			selection_end = selection_start;
			}
		return;
		}
	else if (c < ' ') {
		// Ignore other control characters.
		return;
		}

	// Insert the key.
	std::string new_value = value.substr(0, selection_start) + utf8_for_char(c);
	int new_selection = new_value.size();
	new_value += value.substr(selection_end);
	value = new_value;
	selection_start = selection_end = new_selection;
	tick_phase_time = tick_update_time = TimeSeconds::now();
}

void StringInputBox::special_key_pressed(SpecialKey key)
{
	switch (key) {
		case UpArrow:
		case HomeKey:
			select_start();
			break;
		case DownArrow:
		case EndKey:
			select_end();
			break;
		case LeftArrow:
			if (selection_start == selection_end) {
				selection_start -= 1;
				if (selection_start < 0)
					selection_start = 0;
				// Find start of UTF8 character.
				while (selection_start > 0 && (value[selection_start] & 0xC0) == 0x80)
					selection_start -= 1;
				selection_end = selection_start;
				}
			else
				selection_end = selection_start;
			break;
		case RightArrow:
			if (selection_start == selection_end) {
				int end = value.size();
				selection_end += 1;
				if (selection_end > end)
					selection_end = end;
				// Find end of UTF8 character.
				while (selection_end < end && (value[selection_end] & 0xC0) == 0x80)
					selection_end += 1;
				selection_start = selection_end;
				}
			else
				selection_start = selection_end;
			break;
		default: break;
		}

	tick_phase_time = tick_update_time = TimeSeconds::now();
}


void StringInputBox::mouse_pressed(int x, int y)
{
	if (last_click_time.elapsed_time() > double_click_time)
		clicks = 0;
	clicks += 1;
	last_click_time = TimeSeconds::now();

	if (clicks == 1) {
		drag_start = char_pos_for_mouse_pos(x);
		selection_start = selection_end = drag_start;
		}

	else if (clicks == 2) {
		extend_selection_to_word_start();
		extend_selection_to_word_end();
		drag_start = selection_start;
		drag_start_end = selection_end;
		}

	else
		select_all();
}

bool StringInputBox::mouse_released(int x, int y)
{
	mouse_moved(x, y);
	drag_start = drag_start_end = -1;
	return false;
}

void StringInputBox::mouse_moved(int x, int y)
{
	if (drag_start < 0)
		return;

	if (clicks == 1) {
		int cur_pos = char_pos_for_mouse_pos(x);
		if (cur_pos <= drag_start) {
			selection_start = cur_pos;
			selection_end = drag_start;
			}
		else {
			selection_start = drag_start;
			selection_end = cur_pos;
			}
		}

	else if (clicks == 2) {
		int cur_pos = char_pos_for_mouse_pos(x);
		if (cur_pos < drag_start) {
			selection_start = cur_pos;
			extend_selection_to_word_start();
			}
		else if (cur_pos > drag_start_end) {
			selection_end = cur_pos;
			extend_selection_to_word_end();
			}
		else {
			selection_start = drag_start;
			selection_end = drag_start_end;
			}
		}
}


void StringInputBox::select_start()
{
	selection_start = selection_end = 0;
}

void StringInputBox::select_end()
{
	selection_start = selection_end = value.size();
}

void StringInputBox::select_all()
{
	selection_start = 0;
	selection_end = value.size();
}


double StringInputBox::drawn_label_width()
{
	auto cairo = gui->cairo();
	cairo_save(cairo);
	setup_font();
	cairo_text_extents_t text_extents;
	cairo_text_extents(cairo, label.c_str(), &text_extents);
	cairo_restore(cairo);
	return text_extents.x_advance;
}


StringInputBox::DrawLocs StringInputBox::setup_font()
{
	auto cairo = gui->cairo();
	if (fc_font)
		use_font(fc_font);
	else
		cairo_select_font_face(cairo, (style.font ? style.font : gui->default_font()), CAIRO_FONT_SLANT_NORMAL, style.font_weight);
	cairo_set_font_size(cairo, style.relative_font_size * rect.height);
	cairo_text_extents_t text_extents;
	cairo_text_extents(cairo, "Mg", &text_extents);
	// Center "Mj" vertically.
	double margin = (rect.height - text_extents.height) / 2;
	double baseline = rect.y + margin - text_extents.y_bearing;
	return { margin, baseline };
}


int StringInputBox::char_pos_for_mouse_pos(int x)
{
	auto cairo = gui->cairo();
	cairo_save(cairo);
	auto [ margin, baseline ] = setup_font();
	cairo_text_extents_t text_extents;

	auto cur_label_width = force_label_width;
	if (cur_label_width <= 0.0) {
		cairo_text_extents(cairo, label.c_str(), &text_extents);
		cur_label_width = text_extents.x_advance;
		}

	x -= rect.x + cur_label_width + margin;

	// Binary search for longest string before x.
	// (We want the longest because it could end with a character that takes
	// multiple bytes in UTF8.)
	int left = 0;
	int right = value.size();
	while (left < right) {
		int mid = (left + right) / 2;
		cairo_text_extents(cairo, value.substr(0, mid).c_str(), &text_extents);
		if (text_extents.x_advance < x)
			left = mid + 1;
		else
			right = mid;
		}
	int pos = left;

	cairo_restore(cairo);
	return pos;
}


void StringInputBox::extend_selection_to_word_start()
{
	while (selection_start > 0) {
		if (!is_word_char(value[selection_start - 1]))
			break;
		selection_start -= 1;
		}
}

void StringInputBox::extend_selection_to_word_end()
{
	int end = value.size();
	while (selection_end < end) {
		if (!is_word_char(value[selection_end]))
			break;
		selection_end += 1;
		}
}


std::string StringInputBox::utf8_for_char(unsigned int c)
{
	uint8_t buffer[8];
	uint8_t* out = buffer;

	if (c < 0x80) {
		*out++ = c;
		*out++ = 0;
		return std::string((char*) buffer);
		}

	int shift = 0;
	if (c < 0x0800) {
		*out++ = 0xC0 | (c >> 6);
		shift = 0;
		}
	else if (c < 0x10000) {
		*out++ = 0xE0 | (c >> 12);
		shift = 6;
		}
	else if (c < 0x200000) {
		*out++ = 0xF0 | (c >> 18);
		shift = 12;
		}
	else if (c < 0x4000000) {
		*out++ = 0xF8 | (c >> 24);
		shift = 18;
		}
	else if (c < 0x80000000) {
		*out++ = 0xFC | (c >> 30);
		shift = 24;
		}
	else {
		// Invalid.
		shift = -1;
		}
	while (shift >= 0) {
		*out++ = 0x80 | ((c >> shift) & 0x3F);
		shift -= 6;
		}
	*out++ = 0;
	return std::string((char*) buffer);
}


bool StringInputBox::is_word_char(char c_in)
{
	uint8_t c = c_in;
	return ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c >= 0x80);
}



