#pragma once

#include <xcb/xcb.h>
#include <string>
#include <map>


class XCBConnection {
	public:
		xcb_connection_t* connection = nullptr;
		xcb_screen_t* screen = nullptr;

		bool connect();
		void disconnect();

		void flush() { xcb_flush(connection); }

		void request_atom(const std::string& name);
		xcb_atom_t atom(const std::string& name);

		xcb_visualtype_t* find_visual(xcb_visualid_t visual);

	protected:
		struct Atom {
			xcb_atom_t atom = 0;
			xcb_intern_atom_cookie_t cookie = {};
			};
		std::map<std::string, Atom> atoms;
	};

extern XCBConnection xcb_connection;

