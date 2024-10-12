#pragma once

#include "Widget.h"
#include <string>
#include <stdint.h>

class CompoundWidget;


class CairoWindow {
	public:
		CairoWindow(double initial_width = default_width, double initial_height = default_height);
		virtual ~CairoWindow();
		virtual bool is_valid() = 0;

		virtual CairoGUI* gui() = 0;

		virtual void set_widget(CompoundWidget* new_widget);
		virtual CompoundWidget* replace_widget(CompoundWidget* new_widget);
			// Like set_widget(), but returns any old widget instead of deleting it.

		virtual void redraw();
		virtual void paint();
		virtual void resized(double new_width, double new_height) = 0;
		virtual void mouse_pressed(int32_t x, int32_t y, int button);
		virtual void mouse_released(int32_t x, int32_t y, int button);
		virtual void mouse_moved(int32_t x, int32_t y);
		virtual void key_pressed(int c);
		virtual void special_key_pressed(SpecialKey key);
		virtual int next_update_ms();

		virtual void set_title(const std::string& title) = 0;

		bool redraw_requested = false;

		static double default_width, default_height;
		static Color default_background_color;
		Color background_color = default_background_color;

	protected:
		CompoundWidget* widget = nullptr;
		double width = 0, height = 0;
	};


