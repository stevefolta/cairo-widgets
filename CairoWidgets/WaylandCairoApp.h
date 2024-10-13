#pragma once

#include "CairoApp.h"


class WaylandCairoApp : public CairoApp {
	public:
		WaylandCairoApp();
		~WaylandCairoApp();

		void run();
		CairoWindow* new_window();

	protected:
		bool running = false;
	};

