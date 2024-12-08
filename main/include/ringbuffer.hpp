/* ---------------------------------------------------------------- *\
 * ringbuffer.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-12-08
\* ---------------------------------------------------------------- */
#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP

//#include <stdexcept>
#include <fstream>
#include <vector>

namespace Anthrax
{

template <class T>
class Ringbuffer
{
public:
	Ringbuffer<T>(int size_);
	Ringbuffer<T>() : Ringbuffer<T>(2) {}
	Ringbuffer<T> &operator=(const Ringbuffer<T> &other) { copy(other); return *this; }
	Ringbuffer<T>(const Ringbuffer<T> &other) { copy(other); }
	void resize(size_t new_size);
	void copy(const Ringbuffer<T> &other);
	void push_back(T value);
	T& operator[](unsigned int i);
	size_t size();

private:
	std::vector<T> data_;
	size_t max_size_;
	size_t start_, end_;
};

template <class T>
Ringbuffer<T>::Ringbuffer(int size)
{
	if (size < 2)
	{
		throw std::runtime_error("Ringbuffer size must be at least 2!");
	}
	max_size_ = size;
	start_ = 0;
	end_ = 0;
	data_.resize(max_size_);
	return;
}


template <class T>
void Ringbuffer<T>::resize(size_t new_size)
{
	// TODO: preserve elements
	max_size_ = new_size;
	start_ = 0;
	end_ = 0;
	data_.resize(max_size_);
	return;
}

template <class T>
void Ringbuffer<T>::copy(const Ringbuffer<T> &other)
{
	max_size_ = other.max_size_;
	for (unsigned int i = 0; i < max_size_; i++)
	{
		data_[i] = other.data_[i];
	}
	start_ = other.start_;
	end_ = other.end_;
	return;
}


template <class T>
void Ringbuffer<T>::push_back(T value)
{
	end_ = (end_+1) % max_size_;
	if (end_ == start_)
		start_ = (start_+1) % max_size_;
	data_[end_] = value;
	return;
}


template <class T>
T& Ringbuffer<T>::operator[](unsigned int i)
{
	if (i >= size())
	{
		throw std::runtime_error("Ringbuffer indexed out of bounds!");
	}
	return data_[((start_ + i) % max_size_)];
}


template <class T>
size_t Ringbuffer<T>::size()
{
	if (end_ > start_)
		return (end_ - start_ + 1);
	return ((max_size_ - start_) + end_ + 1);
}

} // namespace Anthrax

#endif // RINGBUFFER_HPP
