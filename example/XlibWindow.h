#include "CompoundWidget.h"
#include "CairoGUI.h"
#include <X11/Xlib.h>
#include <string>


class XlibWindow : public CompoundWidget {
	public:
		XlibWindow(
			Display* x_display, Atom wm_delete_window_atom,
			double initial_width = default_width, double initial_height = default_height);
		virtual ~XlibWindow();
		bool is_valid() { return cairo != nullptr; }

		Window x_window = 0;

		virtual void redraw();
		virtual void paint();
		virtual void resize(double new_width, double new_height);
		virtual void mouse_pressed(int32_t x, int32_t y, int button);
		virtual void mouse_released(int32_t x, int32_t y, int button);
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
				CairoGUI(XlibWindow* window_in)
					: window(window_in) {}
				cairo_t* cairo() { return window->cairo; }
				void refresh() {
					if (window->surface) {
						cairo_surface_flush(window->surface);
						XFlush(window->display);
						}
					}
				Rect popup_limits() {
					return { 0, 0, (double) window->rect.width, (double) window->rect.height };
					}
				XlibWindow* window;
			};

		Display* display = nullptr;
		Atom wm_delete_window_atom = 0;
		CairoGUI cairo_gui;
		cairo_surface_t* surface = nullptr;
		cairo_t* cairo = nullptr;
		Widget* focused_widget = nullptr;
	};

