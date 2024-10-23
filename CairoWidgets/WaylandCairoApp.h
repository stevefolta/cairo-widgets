#pragma once

#include "CairoApp.h"

class WaylandCairoWindow;


class WaylandCairoApp : public CairoApp {
	public:
		WaylandCairoApp();
		~WaylandCairoApp();

		void run();
		CairoWindow* new_window();

	protected:
		static const int buffer_depletion_redraw_ms = 16; 	// ~60fps

		bool running = false;
	};

