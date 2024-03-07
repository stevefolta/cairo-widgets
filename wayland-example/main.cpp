#include "ExampleWindow.h"
#include "WaylandDisplay.h"
#include <poll.h>
#include <iostream>

bool running = true;


int fail(const char* message)
{
	std::cerr << message << std::endl;
	return 1;
}

int main(int argc, const char* argv[])
{
	// Get the connection to the display.
	if (!wayland_display.connect())
		return fail("Can't open display.");

	// Create the window.
	ExampleWindow* example_window = new ExampleWindow();
	if (!example_window->is_valid())
		return fail("Couldn't open window");
	example_window->set_title("CairoWidgets Example");

	// Event loop.
	std::vector<struct pollfd> poll_fds = {
		{ wl_display_get_fd(wayland_display.display), POLLIN, 0 },
		};
	while (running) {
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

	return 0;
}



