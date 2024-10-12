#include "CairoApp.h"


void CairoApp::add_fd(int fd, std::function<void()> read_fn)
{
	client_fds[fd] = read_fn;
	fd_update_needed = true;
}


void CairoApp::remove_fd(int fd)
{
	client_fds.erase(fd);
	fd_update_needed = true;
}



