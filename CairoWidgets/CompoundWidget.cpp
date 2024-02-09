#include "CompoundWidget.h"


CompoundWidget::~CompoundWidget()
{
	for (auto widget: all_widgets)
		delete widget;
}


void CompoundWidget::paint()
{
	// Always draw the tracking_widget on top, since it could be something like
	// a popped-up PopupMenu.
	for (auto widget: all_widgets) {
		if (widget != tracking_widget)
			widget->paint();
		}
	if (tracking_widget)
		tracking_widget->paint();
}


void CompoundWidget::mouse_pressed(int x, int y)
{
	// Check for a "tapped open" widget.
	if (tracking_widget) {
		if (tracking_widget->contains(x, y)) {
			tracking_widget->mouse_pressed(x, y);
			return;
			}
		else
			tracking_widget->mouse_released(x, y);
		}

	tracking_widget = nullptr;
	for (auto widget: all_widgets) {
		if (widget->contains(x, y)) {
			tracking_widget = widget;
			break;
			}
		}
	if (tracking_widget)
		tracking_widget->mouse_pressed(x, y);
}

bool CompoundWidget::mouse_released(int x, int y)
{
	bool accepted = false;
	if (tracking_widget) {
		accepted = tracking_widget->mouse_released(x, y);
		if (accepted) {
			widget_accepted(tracking_widget);
			tracking_widget = nullptr;
			}
		else if (!tracking_widget->sticky_tracking())
			tracking_widget = nullptr;
		}
	return accepted;
}

void CompoundWidget::mouse_moved(int x, int y)
{
	for (auto widget: all_widgets)
		widget->mouse_moved(x, y);
}



