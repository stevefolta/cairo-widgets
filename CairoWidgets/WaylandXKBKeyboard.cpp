#include "WaylandXKBKeyboard.h"
#include <sys/mman.h>
#include <unistd.h>


bool WaylandXKBKeyboard::open()
{
	key_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (key_context == nullptr)
		return false;

	return true;
}


void WaylandXKBKeyboard::close()
{
	if (key_state) {
		xkb_state_unref(key_state);
		key_state = nullptr;
		}
	if (keymap) {
		xkb_keymap_unref(keymap);
		keymap = nullptr;
		}
}


void WaylandXKBKeyboard::use_keymap_fd(int fd, uint32_t size)
{
	char* keymap_shm = (char*) mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (keymap_shm == MAP_FAILED)
		return;
	auto new_keymap =
		xkb_keymap_new_from_string(
			key_context, keymap_shm,
			XKB_KEYMAP_FORMAT_TEXT_V1,
			XKB_KEYMAP_COMPILE_NO_FLAGS);
	munmap(keymap_shm, size);
	::close(fd);
	if (new_keymap == nullptr)
		return;
	auto new_key_state = xkb_state_new(new_keymap);
	if (new_key_state == nullptr) {
		xkb_keymap_unref(new_keymap);
		return;
		}

	if (key_state)
		xkb_state_unref(key_state);
	if (keymap)
		xkb_keymap_unref(keymap);
	keymap = new_keymap;
	key_state = new_key_state;
}


void WaylandXKBKeyboard::update_modifiers(uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	if (keymap == nullptr || key_state == nullptr)
		return;

	xkb_state_update_mask(key_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}


std::string WaylandXKBKeyboard::utf8_for(xkb_keycode_t key)
{
	if (key_state == nullptr)
		return "";
	std::string result(16, 0);
	auto num_bytes = xkb_state_key_get_utf8(key_state, key, result.data(), result.size() - 1);
	result = result.substr(0, num_bytes);
	return result;
}



