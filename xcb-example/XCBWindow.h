#pragma once

#include "CairoGUI.h"
#include "XCBConnection.h"
#include "Widget.h"
#include <xcb/xcb.h>
#include <vector>
#include <string>


class XCBWindow {
	public:
		XCBWindow(double initial_width = default_width, double initial_height = default_height);
		virtual ~XCBWindow();
		bool is_valid() { return cairo != nullptr; }

		xcb_window_t x_window;

		virtual void redraw();
		virtual void paint();
		virtual void resize(double new_width, double new_height);
		virtual void mouse_pressed(int32_t x, int32_t y, int button);
		virtual void mouse_released(int32_t x, int32_t y, int button);
		virtual void mouse_moved(int32_t x, int32_t y);
		virtual void key_pressed(int c);
		virtual void special_key_pressed(SpecialKey key);
		virtual int next_update_ms() { return -1; }

		void set_title(const std::string& title);

		static double default_width, default_height;
		static Color default_background_color;
		Color background_color = default_background_color;

	protected:
		class CairoGUI : public ::CairoGUI {
			public:
				CairoGUI(XCBWindow* window_in)
					: window(window_in) {}
				cairo_t* cairo() { return window->cairo; }
				void refresh() {
					if (window->surface) {
						cairo_surface_flush(window->surface);
						xcb_flush(xcb_connection.connection);
						}
					}
				Rect popup_limits() {
					return { 0, 0, (double) window->width, (double) window->height };
					}
				XCBWindow* window;
			};

		cairo_surface_t* surface = nullptr;
		cairo_t* cairo = nullptr;
		CairoGUI cairo_gui;
		double width, height;
		std::vector<Widget*> all_widgets;
		Widget* tracking_widget = nullptr;
		Widget* focused_widget = nullptr;

		virtual void layout() {}
		virtual void widget_accepted(Widget* widget) {}
	};


