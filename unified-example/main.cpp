#include "ExampleWindowWidget.h"
#include "CairoWindow.h"
#ifdef XCB_WINDOW_BACKEND
	#include "XCBCairoApp.h"
#elif defined(WAYLAND_WINDOW_BACKEND)
	#include "WaylandCairoApp.h"
#endif


int main(int argc, const char** argv)
{
#ifdef XCB_WINDOW_BACKEND
	XCBCairoApp app;
#elif defined(WAYLAND_WINDOW_BACKEND)
	WaylandCairoApp app;
#endif
	auto window = app.new_window();
	window->set_widget(new ExampleWindowWidget(window->gui()));
	window->set_title("CairoWidgets Example");
	app.run();
}

