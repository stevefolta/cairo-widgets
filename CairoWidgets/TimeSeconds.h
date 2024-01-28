#pragma once

#include <time.h>

// We'd just call this "Time", but stupid X11 already defines a type of that
// name.

struct TimeSeconds {
	struct timespec time;

	void clear() {
		this->time.tv_sec = 0;
		this->time.tv_nsec = 0;
		}

	static TimeSeconds now() {
		TimeSeconds result;
		clock_gettime(CLOCK_REALTIME, &result.time);
		return result;
		}

	time_t seconds() { return time.tv_sec; }

	TimeSeconds operator+(const TimeSeconds& other) const;
	TimeSeconds operator-(const TimeSeconds& other) const;

	bool operator==(const TimeSeconds& other) const {
		return time.tv_sec == other.time.tv_sec && time.tv_nsec == other.time.tv_nsec;
		}
	bool operator!=(const TimeSeconds& other) const {
		return time.tv_sec != other.time.tv_sec || time.tv_nsec != other.time.tv_nsec;
		}
	bool operator<(const TimeSeconds& other) const {
		return
			(time.tv_sec < other.time.tv_sec) ||
			(time.tv_sec == other.time.tv_sec && time.tv_nsec < other.time.tv_nsec);
		}
	bool operator<=(const TimeSeconds& other) const {
		return
			(time.tv_sec < other.time.tv_sec) ||
			(time.tv_sec == other.time.tv_sec && time.tv_nsec <= other.time.tv_nsec);
		}
	bool operator>(const TimeSeconds& other) const {
		return other <= *this;
		}
	bool operator>=(const TimeSeconds& other) const {
		return other < *this;
		}

	double as_double() const;

	double elapsed_time() { return (now() - *this).as_double(); }
	};

