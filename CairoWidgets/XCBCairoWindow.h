#pragma once

#include "CairoWindow.h"
#include "CairoGUI.h"
#include "XCBConnection.h"
#include <xcb/xcb.h>


class XCBCairoWindow : public CairoWindow {
	public:
		XCBCairoWindow(double initial_width = default_width, double initial_height = default_height);
		~XCBCairoWindow();
		bool is_valid() { return cairo != nullptr; }

		::CairoGUI* gui() { return &cairo_gui; }
		void resized(double new_width, double new_height);
		void set_title(const std::string& title);

		xcb_window_t x_window = 0;

	protected:
		class CairoGUI : public ::CairoGUI {
			public:
				CairoGUI(XCBCairoWindow* window_in)
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
				XCBCairoWindow* window;
			};

		cairo_surface_t* surface = nullptr;
		cairo_t* cairo = nullptr;
		CairoGUI cairo_gui;
	};

