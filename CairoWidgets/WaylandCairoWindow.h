#pragma once

#include "CairoWindow.h"
#include "CairoGUI.h"
#include "TimeSeconds.h"
#include <vector>


class WaylandCairoWindow : public CairoWindow {
	public:
		WaylandCairoWindow(double initial_width = default_width, double initial_height = default_height);
		~WaylandCairoWindow();
		bool is_valid() { return wayland_surface != nullptr; }

		::CairoGUI* gui() { return &cairo_gui; }
		bool can_redraw();

		void redraw();
		void paint();
		void set_title(const std::string& title);
		void resized(double new_width, double new_height);

		void mouse_pressed(double x, double y, int button, uint32_t serial);
		void mouse_released(double x, double y, int button);
		void mouse_moved(int32_t x, int32_t y);
		void entered(double x, double y);
		void scroll_up(double x, double y);
		void scroll_down(double x, double y);

		struct wl_surface* wayland_surface = nullptr;

		struct Backend {
			virtual ~Backend() {}
			virtual cairo_surface_t* prepare(int width, int height) = 0;
			virtual void swap() = 0;
			virtual TimeSeconds next_idle_time() { return { 0, 0 }; }
			virtual void idle(int width, int height) {}
			};

		struct Style {
			Color default_background_color = { 1.0, 1.0, 1.0, 1.0 };
			double 
				frame_left = 6, frame_right = 6,
				frame_top = 40, frame_bottom = 6;
			Color frame_color = { 0.25, 0.25, 0.25, 1.0 };
			Color title_color = { 1.0, 1.0, 1.0, 1.0 };
			const char* title_font = nullptr;
			double title_font_size = 16;
			cairo_font_weight_t title_font_weight = CAIRO_FONT_WEIGHT_BOLD;
			double frame_button_size = 16, frame_button_margin = 10, frame_button_spacing = 6;
			double frame_button_border_size = 1;
			double frame_button_icon_inset = 4, frame_button_icon_line_width = 2;
			Color 
				frame_button_idle_color = { 0.95, 0.95, 0.95 },
				frame_button_clicked_color = { 0.50, 0.50, 0.50 },
				frame_button_border_color = { 0, 0, 0 },
				frame_button_icon_color = { 0, 0, 0 };
			};
		static Style style;

	protected:
		class CairoGUI : public ::CairoGUI {
			public:
				CairoGUI(WaylandCairoWindow* window_in)
					: window(window_in) {}
				cairo_t* cairo() { return window->get_cairo(); }
				void refresh() { window->refresh(); }
				Rect popup_limits() {
					return { 0, 0, (double) window->width, (double) window->height };
					}
				WaylandCairoWindow* window;
			};

		struct xdg_surface* xdg_surface = nullptr;
		struct xdg_toplevel* xdg_toplevel = nullptr;
		struct zxdg_toplevel_decoration_v1* xdg_decoration_v1 = nullptr;
		Backend* backend = nullptr;
		cairo_surface_t* cairo_surface = nullptr;
		cairo_t* cairo = nullptr;
		cairo_surface_t* null_surface = nullptr;
		cairo_t* null_cairo = nullptr;
		CairoGUI cairo_gui;
		Widget* focused_widget = nullptr;
		std::string title;
		bool is_maximized = false, is_fullscreen = false, is_focused = false, is_resizing = false;
		bool server_side_decorations = false;
		int last_cursor = -1;

		void refresh();
		cairo_t* get_cairo();
		virtual void resize(int32_t new_width, int32_t new_height, struct wl_array* states);
		virtual void surface_enter(struct wl_output* output) {}
		virtual void surface_leave(struct wl_output* output) {}
		virtual void close_requested();
		void draw_frame();
		void frame_mouse_pressed(double x, double y, uint32_t serial);
		void frame_mouse_moved(double x, double y);
		void frame_mouse_released(double x, double y);
		void layout_frame_buttons();

		struct FrameButton {
			enum Type {
				None, Close, Minimize, Maximize,
				};
			enum State {
				Idle, Clicked,
				};
			Type type;
			State state = Idle;
			Rect rect;
			FrameButton(Type type_in) : type(type_in) {}
			void draw(cairo_t* cairo);
			};
		std::vector<FrameButton> frame_buttons;
		FrameButton* tracking_frame_button = nullptr;
	};

