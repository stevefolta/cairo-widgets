#pragma once

#include <xkbcommon/xkbcommon-x11.h>
#include <string>

// Modeled after libxkbcommon's example code:
//  	<https://github.com/xkbcommon/libxkbcommon/blob/master/tools/interactive-x11.c>
// Requires the following libraries:
//  	xkbcommon xkbcommon-x11 xcb-xkb


class XKBKeyboard {
	public:
		XKBKeyboard() {}
		~XKBKeyboard() { close(); }

		bool open(xcb_connection_t* xcb_connection_in);
		void close();

		uint8_t xkb_response_type = -1;

		void handle_event(xcb_generic_event_t* event);

		uint32_t unicode_for(xkb_keycode_t key) {
			return key_state ? xkb_state_key_get_utf32(key_state, key) : 0;
			}
		std::string utf8_for(xkb_keycode_t key);
		xkb_keysym_t keysym_for(xkb_keycode_t key) {
			return key_state ? xkb_state_key_get_one_sym(key_state, key) : 0;
			}

	protected:
		xcb_connection_t* xcb_connection = nullptr;
		int32_t keyboard_id;
		struct xkb_context* key_context = nullptr;
		struct xkb_keymap* keymap = nullptr;
		struct xkb_state* key_state = nullptr;

		bool update_keymap();
	};

