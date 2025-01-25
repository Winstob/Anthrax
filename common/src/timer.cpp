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
	running_ = false;
	return;
}


void Timer::copy(const Timer &other)
{
	interval_ = other.interval_;
	start_time_point_ = other.start_time_point_;
	stop_time_point_ = other.stop_time_point_;
	running_ = other.running_;
	return;
}


void Timer::start()
{
	start_time_point_ = std::chrono::steady_clock::now();
	running_ = true;
	return;
}

long long Timer::stop()
{
	stop_time_point_ = std::chrono::steady_clock::now();
	running_ = false;
	return query();
}


long long Timer::query()
{
	long long time;
	std::chrono::time_point<std::chrono::steady_clock> stop_time_point;
	if (running_)
	{
		stop_time_point = std::chrono::steady_clock::now();
	}
	else
	{
		stop_time_point = stop_time_point_;
	}
	auto time_elapsed = stop_time_point - start_time_point_;
	switch (interval_)
	{
		case SECONDS:
			time = static_cast<long long>
					(std::chrono::duration_cast<std::chrono::seconds>
					(time_elapsed).count());
			break;
		case MILLISECONDS:
			time = static_cast<long long>
					(std::chrono::duration_cast<std::chrono::milliseconds>
					(time_elapsed).count());
			break;
		case MICROSECONDS:
			time = static_cast<long long>
					(std::chrono::duration_cast<std::chrono::microseconds>
					(time_elapsed).count());
			break;
		default:
			time = static_cast<long long>
					(std::chrono::duration_cast<std::chrono::milliseconds>
					(time_elapsed).count());
			break;
	}
	return time;
}

} // namespace Anthrax
