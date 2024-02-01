#pragma once


struct Rect {
	double x, y;
	double width, height;

	bool contains(double pt_x, double pt_y) {
		return
			pt_x >= x && pt_x < x + width &&
			pt_y >= y && pt_y < y + height;
		}
	};

