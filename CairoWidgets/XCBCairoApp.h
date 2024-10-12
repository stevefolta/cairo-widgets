#pragma once

#include "CairoApp.h"
#include "XCBConnection.h"
#include "XCBXKBKeyboard.h"
#include "Rect.h"
#include "Widget.h"
#include <xcb/xcb.h>
#include <string>
#include <map>
#include <vector>

class XCBWindow;
class XCBCairoWindow;
class CairoDrawer;
class CairoFont;


class XCBCairoApp : public CairoApp {
	public:
		XCBCairoApp();
		~XCBCairoApp();

		CairoWindow* new_window();
		void run();

	protected:
		enum {
			update_ms = 500,
			};

		struct Atom {
			xcb_atom_t atom = 0;
			xcb_intern_atom_cookie_t cookie = {};
			};

		XCBXKBKeyboard keyboard;
		std::map<std::string, Atom> atoms;
		std::map<xcb_window_t, XCBCairoWindow*> windows_by_id;
		bool running = false;

		XCBCairoWindow* window_for(xcb_window_t x_window);
		void handle_x11_event(xcb_generic_event_t* event);
		void handle_key_event(xcb_key_press_event_t* event);
	};

