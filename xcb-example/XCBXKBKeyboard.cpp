#include "XCBXKBKeyboard.h"

// XCB doesn't like C++ for some reason...
#define explicit explicit_
#include <xcb/xkb.h>
#undef explicit


bool XCBXKBKeyboard::open(xcb_connection_t* xcb_connection_in)
{
	xcb_connection = xcb_connection_in;
	auto result =
		xkb_x11_setup_xkb_extension(
			xcb_connection,
			XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
			XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
			nullptr, nullptr, &xkb_response_type, nullptr);
	if (!result) {
		xcb_connection = nullptr;
		return false;
		}
	key_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (key_context == nullptr)
		return false;
	keyboard_id = xkb_x11_get_core_keyboard_device_id(xcb_connection);
	if (keyboard_id == -1)
		return false;
	if (!update_keymap())
		return false;

	// Select events.
	auto required_events =
		XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
		XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
		XCB_XKB_EVENT_TYPE_STATE_NOTIFY;
	auto required_nkn_details = XCB_XKB_NKN_DETAIL_KEYCODES;
	auto required_state_details =
		XCB_XKB_STATE_PART_MODIFIER_BASE |
		XCB_XKB_STATE_PART_MODIFIER_LATCH |
		XCB_XKB_STATE_PART_MODIFIER_LOCK |
		XCB_XKB_STATE_PART_GROUP_BASE |
		XCB_XKB_STATE_PART_GROUP_LATCH |
		XCB_XKB_STATE_PART_GROUP_LOCK;
	auto required_map_parts =
		XCB_XKB_MAP_PART_KEY_TYPES |
		XCB_XKB_MAP_PART_KEY_SYMS |
		XCB_XKB_MAP_PART_MODIFIER_MAP |
		XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
		XCB_XKB_MAP_PART_KEY_ACTIONS |
		XCB_XKB_MAP_PART_VIRTUAL_MODS |
		XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP;
	xcb_xkb_select_events_details_t details = {};
	details.affectNewKeyboard = details.newKeyboardDetails = required_nkn_details;
	details.affectState = details.stateDetails = required_state_details;
	xcb_xkb_select_events_aux(
		xcb_connection, keyboard_id,
		required_events, 0, 0, required_map_parts, required_map_parts, &details);

	return true;
}


void XCBXKBKeyboard::close()
{
	if (key_state) {
		xkb_state_unref(key_state);
		key_state = nullptr;
		}
	if (keymap) {
		xkb_keymap_unref(keymap);
		keymap = nullptr;
		}
	xkb_response_type = -1;
	xcb_connection = nullptr;
}


void XCBXKBKeyboard::handle_event(xcb_generic_event_t* event)
{
	if (event->response_type != xkb_response_type)
		return;
	// All the xcb_xkb_*_event_t's have "deviceID" and "xkbType" in the same place.
	if (((xcb_xkb_new_keyboard_notify_event_t*) event)->deviceID != keyboard_id)
		return;

	switch (((xcb_xkb_new_keyboard_notify_event_t*) event)->xkbType) {
		case XCB_XKB_NEW_KEYBOARD_NOTIFY:
			if (((xcb_xkb_new_keyboard_notify_event_t*) event)->changed & XCB_XKB_NKN_DETAIL_KEYCODES)
				update_keymap();
			break;
		case XCB_XKB_MAP_NOTIFY:
			update_keymap();
			break;
		case XCB_XKB_STATE_NOTIFY:
			{
			auto state_event = (xcb_xkb_state_notify_event_t*) event;
			xkb_state_update_mask(
				key_state,
				state_event->baseMods, state_event->latchedMods, state_event->lockedMods,
				state_event->baseGroup, state_event->latchedGroup, state_event->lockedGroup);
			}
			break;
		}
}


std::string XCBXKBKeyboard::utf8_for(xkb_keycode_t key)
{
	if (key_state == nullptr)
		return "";
	std::string result(16, 0);
	auto num_bytes = xkb_state_key_get_utf8(key_state, key, result.data(), result.size() - 1);
	result = result.substr(0, num_bytes);
	return result;
}


bool XCBXKBKeyboard::update_keymap()
{
	auto new_keymap =
		xkb_x11_keymap_new_from_device(
			key_context, xcb_connection, keyboard_id, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (new_keymap == nullptr)
		return false;
	auto new_key_state = xkb_x11_state_new_from_device(new_keymap, xcb_connection, keyboard_id);
	if (new_key_state == nullptr) {
		xkb_keymap_unref(new_keymap);
		return false;
		}
	xkb_state_unref(key_state);
	xkb_keymap_unref(keymap);
	keymap = new_keymap;
	key_state = new_key_state;
	return true;
}



