#pragma once

#include "CairoWindow.h"
#include "CairoGUI.h"
#include <X11/Xlib.h>


class XlibCairoWindow : public CairoWindow {
	public:
		XlibCairoWindow(
			Display* x_display_in, Atom wm_delete_window_atom_in,
			double initial_width = default_width, double initial_height = default_height
			);
		~XlibCairoWindow();
		bool is_valid();

		::CairoGUI* gui() { return &cairo_gui; }
		void resized(double new_width, double new_height);
		void set_title(const std::string& title);

		Window x_window = 0;

	protected:
		class CairoGUI : public ::CairoGUI {
			public:
				CairoGUI(XlibCairoWindow* window_in)
					: window(window_in) {}
				cairo_t* cairo() { return window->cairo; }
				void refresh() {
					if (window->surface) {
						cairo_surface_flush(window->surface);
						XFlush(window->display);
						}
					}
				Rect popup_limits() {
					return { 0, 0, (double) window->width, (double) window->height };
					}
				XlibCairoWindow* window;
			};

		Display* display = nullptr;
		Atom wm_delete_window_atom = 0;
		CairoGUI cairo_gui;
		cairo_surface_t* surface = nullptr;
		cairo_t* cairo = nullptr;
	};

