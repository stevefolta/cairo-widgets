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


bool CompoundWidget::contains(double x, double y)
{
	// A popped-up widget may go beyond the bounds of the rect.
	return rect.contains(x, y) || (tracking_widget && tracking_widget->contains(x, y));
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

	// Find the widget.
	// Go through all_widgets backwards, so widgets in front get checked first.
	tracking_widget = nullptr;
	for (auto it = all_widgets.rbegin(); it != all_widgets.rend(); ++it) {
		auto widget = *it;
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

bool CompoundWidget::sticky_tracking()
{
	// "Stickiness" needs to propagate upward.  That is, if this CompoundWidget
	// has a "sticky" element still tracking after mouse_released(), it needs to
	// remain tracking itself.
	return (tracking_widget != nullptr);
}


void CompoundWidget::remove_widget(Widget* widget)
{
	// Delete the widget from the vector (C++ didn't get std::erase() until C++20).
	for (auto it = all_widgets.begin(); it != all_widgets.end(); ++it) {
		if (*it == widget) {
			all_widgets.erase(it);
			break;
			}
		}
}

void CompoundWidget::delete_widget(Widget* widget)
{
	remove_widget(widget);
	delete widget;
}



