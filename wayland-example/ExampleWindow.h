#pragma once

#include "WaylandWindow.h"
#include "Widget.h"
#include <vector>
#include <stdint.h>

class CairoGUI;
class Button;
class PopupMenu;
class CheckedPopupMenu;
class StringInputBox;
class Checkbox;


class ExampleWindow : public WaylandWindow {
	public:
		ExampleWindow();
		~ExampleWindow();

		int next_update_ms();

	protected:
		Button* button = nullptr;
		PopupMenu* menu = nullptr;
		CheckedPopupMenu* color_menu = nullptr;
		Checkbox* checkbox = nullptr;
		PopupMenu* low_menu = nullptr;
		std::vector<PopupMenu*> unaligned_popups;
		std::vector<PopupMenu*> aligned_popups;
		StringInputBox* string_input_box = nullptr;
		Button* quit_button = nullptr;

		void layout();
		void widget_accepted(Widget* widget);
	};

