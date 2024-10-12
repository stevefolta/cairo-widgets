#pragma once

#include <unistd.h>


class SimpleSignal {
	public:
		SimpleSignal() {
			int pipe_fds[2];
			if (pipe(pipe_fds) == 0) {
				receive_fd = pipe_fds[0];
				send_fd = pipe_fds[1];
				}
			}
		~SimpleSignal() {
			if (send_fd >= 0)
				close(send_fd);
			if (receive_fd >= 0)
				close(receive_fd);
			}
		bool is_valid() { return send_fd >= 0 && receive_fd >= 0; }

		int poll_fd() { return receive_fd; }

		void send(char message = 'Q') {
			write(send_fd, &message, 1);
			}
		char message() {
			char buffer[4];
			if (read(receive_fd, buffer, 1) < 1)
				return 0;
			return buffer[0];
			}

	protected:
		int send_fd = -1, receive_fd = -1;
	};


