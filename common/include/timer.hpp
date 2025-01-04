/* ---------------------------------------------------------------- *\
 * timer.hpp
 * Author: Gavin Ralston
 * Date Created: 2025-1-04
\* ---------------------------------------------------------------- */
#ifndef TIMER_HPP
#define TIMER_HPP

#include <cstddef>
#include <stdexcept>
#include <chrono>

#include "tools.hpp"

namespace Anthrax
{

class Timer
{
public:
	enum Interval
	{
		SECONDS,
		MILLISECONDS
	};
	Timer(Interval interval);
	Timer() : Timer(MILLISECONDS) {};
	Timer &operator=(const Timer &other) { copy(other); return *this; }
	Timer(const Timer &other) { copy(other); }
	void copy(const Timer &other);

	void start();
	long long stop();

private:
	Interval interval_;
	std::chrono::time_point<std::chrono::steady_clock> start_time_point_;
	long long time_;
};

} // namespace Anthrax

#endif // TIMER_HPP
