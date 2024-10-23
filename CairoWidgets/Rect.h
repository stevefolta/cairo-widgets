#pragma once


struct Rect {
	double x, y;
	double width, height;

	bool contains(double pt_x, double pt_y) {
		return
			pt_x >= x && pt_x < x + width &&
			pt_y >= y && pt_y < y + height;
		}

	void shrink_left_by(double amount) {
		x += amount;
		width -= amount;
		}
	void shrink_top_by(double amount) {
		y += amount;
		height -= amount;
		}

	double left() { return x + width; }
	double bottom() { return y + height; }
	};

