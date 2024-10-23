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
		{ signal.poll_fd(), POLLIN, 0 },
		};
	running = true;
	while (running) {
		bool could_redraw = wayland_display.redraw_pending_windows();

		if (fd_update_needed) {
			poll_fds.resize(2);
			for (auto& it: client_fds)
				poll_fds.push_back({ it.first, POLLIN, 0 });
			fd_update_needed = false;
			}

		// Wait for Wayland event, or timeout.
		wl_display_flush(wayland_display.display);
		auto wait_ms = wayland_display.next_update_ms();
		if (!could_redraw) {
			if (wait_ms < 0 || buffer_depletion_redraw_ms < wait_ms)
				wait_ms = buffer_depletion_redraw_ms;
			}
		int num_waiting = poll(poll_fds.data(), poll_fds.size(), wait_ms);
		if (num_waiting <= 0)
			continue;

		if ((poll_fds[0].revents & POLLIN) != 0) {
			wl_display_dispatch(wayland_display.display);
			if (!wayland_display.has_windows()) {
				// Last window has exited.
				running = false;
				}
			}

		// Client fds.
		for (int i = 2; i < (int) poll_fds.size(); ++i) {
			if ((poll_fds[i].revents & POLLIN) != 0) {
				auto it = client_fds.find(poll_fds[i].fd);
				if (it != client_fds.end())
					it->second();
				}
			}

		// Quit and refresh signals.
		if ((poll_fds[1].revents & POLLIN) != 0) {
			// Consolidate multiple refresh requests.
			struct pollfd just_signal_poll_fd = { signal.poll_fd(), POLLIN, 0 };
			while (poll(&just_signal_poll_fd, 1, 0) > 0) {
				auto message = signal.message();
				if (message == 'Q')
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



