/* ---------------------------------------------------------------- *\
 * timer.cpp
 * Author: Gavin Ralston
 * Date Created: 2025-01-04
\* ---------------------------------------------------------------- */

#include "timer.hpp"

namespace Anthrax
{

Timer::Timer(Interval interval)
{
	interval_ = interval;
	return;
}


void Timer::copy(const Timer &other)
{
	interval_ = other.interval_;
	return;
}


void Timer::start()
{
	start_time_point_ = std::chrono::steady_clock::now();
	return;
}

long long Timer::stop()
{
	std::chrono::time_point<std::chrono::steady_clock> stop_time_point =
			std::chrono::steady_clock::now();
	auto time_elapsed = stop_time_point - start_time_point_;
	switch (interval_)
	{
		case SECONDS:
			time_ = static_cast<long long>(std::chrono::duration_cast<std::chrono::seconds>
					(time_elapsed).count());
			break;
		case MILLISECONDS:
			time_ = static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>
					(time_elapsed).count());
			break;
		default:
			time_ = static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>
					(time_elapsed).count());
			break;
	}
	return time_;
}

} // namespace Anthrax
