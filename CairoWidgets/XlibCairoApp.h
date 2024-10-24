#pragma once

#include "CairoApp.h"
#include <X11/Xlib.h>
#include <map>

class XlibCairoWindow;


class XlibCairoApp : public CairoApp {
	public:
		XlibCairoApp();
		~XlibCairoApp();

		void run();
		CairoWindow* new_window();

	protected:
		Display* display = nullptr;
		Atom wm_delete_window_atom = 0;
		std::map<Window, XlibCairoWindow*> windows_by_id;
		bool running = false;

		XlibCairoWindow* window_for(Window x_window);
		void handle_x11_event(XEvent* event);
		void handle_key_event(XKeyEvent* event);
	};

