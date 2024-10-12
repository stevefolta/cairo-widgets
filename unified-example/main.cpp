#include "ExampleWindowWidget.h"
#include "CairoWindow.h"
#ifdef XCB_WINDOW_BACKEND
	#include "XCBCairoApp.h"
#endif


int main(int argc, const char** argv)
{
#ifdef XCB_WINDOW_BACKEND
	XCBCairoApp app;
#endif
	auto window = app.new_window();
	window->set_widget(new ExampleWindowWidget(window->gui()));
	app.run();
}

