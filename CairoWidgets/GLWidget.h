#pragma once

#include "Widget.h"

struct GLWidgetPrivate;


class GLWidget : public Widget {
	public:
		GLWidget(CairoGUI* gui_in, Rect rect_in = {});
		~GLWidget();
		bool is_valid() { return x_window != nullptr; }

		void paint();
		void layout();

		virtual void gl_paint() = 0;

	protected:
		GLWidgetPrivate* x_window = nullptr;

		void glx_start();
	};

