#include "SimpleLabeledPopupMenu.h"
#include "SimplePopupMenu.h"
#include "SimplePopupMenuLabel.h"


SimpleLabeledPopupMenu::SimpleLabeledPopupMenu(
	CairoGUI* gui_in,
	std::string label_in,
	const std::vector<std::string>& items,
	Rect rect_in)
	: Widget(gui_in, rect_in)
{
	menu = new SimplePopupMenu(gui, items);
	label = new SimplePopupMenuLabel(gui, label_in, menu);
	// layout() must be called explicitly.
}


SimpleLabeledPopupMenu::~SimpleLabeledPopupMenu()
{
	delete menu;
	delete label;
}


void SimpleLabeledPopupMenu::paint()
{
	label->paint();
	menu->paint();
}


void SimpleLabeledPopupMenu::mouse_pressed(int x, int y)
{
	if (menu->contains(x, y)) {
		menu->mouse_pressed(x, y);
		tracking_menu = true;
		}
}


bool SimpleLabeledPopupMenu::mouse_released(int x, int y)
{
	bool accepted = false;
	if (tracking_menu)
		accepted = menu->mouse_released(x, y);
	tracking_menu = false;
	return accepted;
}


void SimpleLabeledPopupMenu::mouse_moved(int x, int y)
{
	if (tracking_menu)
		menu->mouse_moved(x, y);
}


void SimpleLabeledPopupMenu::layout(double label_width)
{
	menu->rect = label->rect = rect;
	if (label_width <= 0.0)
		label_width = label->drawn_width();
	label->rect.width = label_width;
	menu->rect.x += label_width;
	menu->rect.width -= label_width;
}


int SimpleLabeledPopupMenu::selected_item()
{
	 return menu->selected_item;
}


double SimpleLabeledPopupMenu::label_width()
{
	menu->rect.height = rect.height;
	return label->drawn_width();
}




