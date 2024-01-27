#include "GLWidget.h"
#include "CairoGUI.h"
#include <cairo/cairo-xlib.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

// We keep some things out of the .h file, since Xlib.h ends up defining some
// names that we don't want to infect others with.
struct GLWidgetPrivate {
	Display* display = nullptr;
	Window window = 0;
	GLXContext gl_context;
	};


GLWidget::GLWidget(CairoGUI* gui_in, Rect rect_in)
	: Widget(gui_in, rect_in)
{
	// Make sure it's an X window.
	x_window = new GLWidgetPrivate();
	auto surface = cairo_get_target(gui->cairo());
	x_window->display = cairo_xlib_surface_get_display(surface);
	bool ok = (x_window->display != nullptr);

	// Initial GL.
	XVisualInfo* visual_info = nullptr;
	if (ok) {
		GLint gl_attributes[] = { GLX_RGBA, GLX_DOUBLEBUFFER, None };
		visual_info = glXChooseVisual(x_window->display, 0, gl_attributes);
		if (visual_info == nullptr)
			ok = false;
		}

	// Create X Window.
	bool created_window = false;
	if (ok) {
		auto width = rect.width, height = rect.height;
		if (width <= 0 || height <= 0) {
			// No size defined yet.  Give it *something*...
			width = height = 100;
			}
		XSetWindowAttributes x_attributes;
		x_attributes.event_mask = CWEventMask;
		x_window->window =
			XCreateWindow(
				x_window->display, cairo_xlib_surface_get_drawable(surface),
				rect.x, rect.y, width, height,
				0, CopyFromParent, CopyFromParent, CopyFromParent,
				CWEventMask, &x_attributes);
		XMapWindow(x_window->display, x_window->window);
		created_window = true;
		}

	// GL context.
	bool created_glx_context = false;
	if (ok) {
		x_window->gl_context = glXCreateContext(x_window->display, visual_info, nullptr, GL_TRUE);
		ok = created_glx_context = (x_window->gl_context != nullptr);
		}

	if (!ok) {
		if (created_glx_context)
			glXDestroyContext(x_window->display, x_window->gl_context);
		if (created_window)
			XDestroyWindow(x_window->display, x_window->window);
		delete x_window;
		x_window = nullptr;
		}
}

GLWidget::~GLWidget()
{
	if (x_window) {
		glXDestroyContext(x_window->display, x_window->gl_context);
		XDestroyWindow(x_window->display, x_window->window);
		delete x_window;
		}
}


void GLWidget::paint()
{
	if (!is_valid())
		return;

	glx_start();
	glViewport(0, 0, rect.width, rect.height);
	gl_paint();
	glXSwapBuffers(x_window->display, x_window->window);
}


void GLWidget::layout()
{
	if (!is_valid())
		return;

	XWindowChanges changes;
	changes.x = rect.x;
	changes.y = rect.y;
	changes.width = rect.width;
	changes.height = rect.height;
	XConfigureWindow(x_window->display, x_window->window, CWX | CWY | CWWidth | CWHeight, &changes);
}


void GLWidget::glx_start()
{
	glXMakeCurrent(x_window->display, x_window->window, x_window->gl_context);
}


