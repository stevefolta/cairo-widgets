#pragma once

#include "SimpleSignal.h"
#include <map>
#include <functional>

class CairoWindow;


class CairoApp {
	public:
		virtual ~CairoApp() {}

		virtual void run() = 0;
		virtual CairoWindow* new_window() = 0;

		void add_fd(int fd, std::function<void()> read_fn);
		void remove_fd(int fd);

		void trigger_redraws() { signal.send('r'); }

	protected:
		SimpleSignal signal;
		std::map<int, std::function<void()>> client_fds;
		bool fd_update_needed = true;
	};

