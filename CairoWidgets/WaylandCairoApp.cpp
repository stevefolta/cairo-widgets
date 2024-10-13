#include "WaylandCairoApp.h"
#include "WaylandDisplay.h"
#include "WaylandCairoWindow.h"
#include <poll.h>
#include <iostream>


WaylandCairoApp::WaylandCairoApp()
{
	// Get the connection to the display.
	if (!wayland_display.connect()) {
		std::cerr << "Can't open Wayland display." << std::endl;
		return;
		}
}

WaylandCairoApp::~WaylandCairoApp()
{
	wayland_display.disconnect();
}



void WaylandCairoApp::run()
{
	// Event loop.
	std::vector<struct pollfd> poll_fds = {
		{ wl_display_get_fd(wayland_display.display), POLLIN, 0 },
		};
	running = true;
	while (running) {
		if (fd_update_needed) {
			poll_fds.resize(1);
			for (auto& it: client_fds)
				poll_fds.push_back({ it.first, POLLIN, 0 });
			}

		// Wait for Wayland event, or timeout.
		wl_display_flush(wayland_display.display);
		int num_waiting = poll(poll_fds.data(), poll_fds.size(), wayland_display.next_update_ms());
		wayland_display.redraw_pending_windows();
		if (num_waiting <= 0)
			continue;

		if ((poll_fds[0].revents & POLLIN) != 0) {
			wl_display_dispatch(wayland_display.display);
			if (!wayland_display.has_windows()) {
				// Last window has exited.
				running = false;
				}
			}
		}

	// Clean up.
	wayland_display.disconnect();
}


CairoWindow* WaylandCairoApp::new_window()
{
	return new WaylandCairoWindow();
}



