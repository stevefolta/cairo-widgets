#include "XCBConnection.h"

XCBConnection xcb_connection;


bool XCBConnection::connect()
{
	connection = xcb_connect(nullptr, nullptr);
	if (xcb_connection_has_error(connection)) {
		connection = nullptr;
		return false;
		}
	screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

	// Get commonly-needed atoms.
	std::map<std::string, Atom> atoms = {
		{ "WM_PROTOCOLS", {} }, { "WM_DELETE_WINDOW", {} }, { "UTF8_STRING", {} }, { "WM_NAME", {} },
		};
	// Send requests.
	for (auto& it: atoms)
		it.second.cookie = xcb_intern_atom(connection, 0, it.first.size(), it.first.data());
	// Get replies.
	for (auto& it: atoms) {
		auto intern_reply = xcb_intern_atom_reply(connection, it.second.cookie, nullptr);
		if (intern_reply) {
			it.second.atom = intern_reply->atom;
			free(intern_reply);
			}
		}

	return true;
}

void XCBConnection::disconnect()
{
	if (connection) {
		xcb_disconnect(connection);
		connection = nullptr;
		}
}


void XCBConnection::request_atom(const std::string& name)
{
	auto& entry = atoms[name];
	if (entry.atom != 0) {
		// Already got it.
		return;
		}
	entry.cookie = xcb_intern_atom(connection, 0, name.size(), name.data());
}

xcb_atom_t XCBConnection::atom(const std::string& name)
{
	auto& entry = atoms[name];
	if (entry.atom != 0) {
		// Already got it.
		return entry.atom;
		}
	entry.cookie = xcb_intern_atom(connection, 0, name.size(), name.data());
	auto intern_reply = xcb_intern_atom_reply(connection, entry.cookie, nullptr);
	if (intern_reply) {
		entry.atom = intern_reply->atom;
		free(intern_reply);
		}
	return entry.atom;
}


xcb_visualtype_t* XCBConnection::find_visual(xcb_visualid_t visual) {
	// Based on <https://www.cairographics.org/cookbook/xcbsurface.c/>.
	auto screen_iterator = xcb_setup_roots_iterator(xcb_get_setup(connection));
	for (; screen_iterator.rem; xcb_screen_next(&screen_iterator)) {
		auto depth_iterator = xcb_screen_allowed_depths_iterator(screen_iterator.data);
		for (; depth_iterator.rem; xcb_depth_next(&depth_iterator)) {
			auto visual_iterator = xcb_depth_visuals_iterator(depth_iterator.data);
			for (; visual_iterator.rem; xcb_visualtype_next(&visual_iterator)) {
				if (visual == visual_iterator.data->visual_id)
					return visual_iterator.data;
				}
			}
		}
	return nullptr;
	}
