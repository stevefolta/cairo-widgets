#pragma once

#include <xkbcommon/xkbcommon.h>
#include <stdint.h>
#include <string>


class XKBKeyboard {
	public:
		XKBKeyboard() {}
		~XKBKeyboard() { close(); }

		bool open();
		void close();

		void use_keymap_fd(int fd, uint32_t size);
		void update_modifiers(uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);

		uint32_t unicode_for(xkb_keycode_t key) {
			return key_state ? xkb_state_key_get_utf32(key_state, key) : 0;
			}
		std::string utf8_for(xkb_keycode_t key);
		xkb_keysym_t keysym_for(xkb_keycode_t key) {
			return key_state ? xkb_state_key_get_one_sym(key_state, key) : 0;
			}

	protected:
		struct xkb_context* key_context = nullptr;
		struct xkb_keymap* keymap = nullptr;
		struct xkb_state* key_state = nullptr;
	};

